#include "miros.h"

#include <stdint.h>

#include "std.h"
#include "stm32.h"

#define LOG2(x) (32 - __builtin_clz(x))
#define DIMENSION(x) (sizeof(x) / sizeof(x[0]))

static thread_t* os_threads[1 + 32]; // array of threads started so far
static thread_t* volatile os_current; // pointer to the current thread
static thread_t* volatile os_next; // pointer to the next thread to run
static uint32_t os_ready; // bitmask of threads that are ready to run
static uint32_t os_delayed; // bitmask of threads that are delayed

static uint32_t idle_stack[40];
static thread_t idle_thread;
static void idle_main(void) {
	for (;;);
}

static void os_sched(void) {
	thread_t* next = os_threads[LOG2(os_ready)];
	if (next != os_current) {
		os_next = next;
		SCB->icsr |= SCB_ICSR_PENDSVSET;
		asm volatile ("dsb");
	}
}

static void os_tick(void) {
	uint32_t workingSet = os_delayed;
	while (workingSet != 0) {
		thread_t* thread = os_threads[LOG2(workingSet)];
		OS_ASSERT((thread != (thread_t*) 0) && (thread->timeout != 0));
		uint32_t bit = (1 << (thread->priority - 1));
		if (--thread->timeout == 0) {
			os_ready |= bit;
			os_delayed &= ~bit;
		}
		workingSet &= ~bit;
	}
}

void os_init(void) {
	nvic_set_priority(IRQN_PENDSV, 0xFF);
	thread_init(&idle_thread, 0, &idle_main, idle_stack, sizeof(idle_stack));
}

void os_run(void) {
	rcc_init();
	systick_init(72e6 / OS_ONE_SECOND);
	nvic_set_priority(IRQN_SYSTICK, 0x00);
	__disable_irq();
	os_sched();
	__enable_irq();
	OS_ERROR();
}

void os_yield(void) {
	__disable_irq();
	uint32_t bit = (1 << (os_current->priority - 1));
	os_ready &= ~bit;
	os_sched();
	os_ready |= bit;
	__enable_irq();
}

void os_delay(uint32_t ticks) {
	__disable_irq();
	OS_ASSERT(os_current != os_threads[0]);
	os_current->timeout = ticks;
	uint32_t bit = (1 << (os_current->priority - 1));
	os_ready &= ~bit;
	os_delayed |= bit;
	os_sched();
	__enable_irq();
}

void os_exit(void) {
	__disable_irq();
	OS_ASSERT(os_current != os_threads[0]);
	uint32_t bit = (1 << (os_current->priority - 1));
	os_ready &= ~bit;
	os_threads[os_current->priority] = (thread_t*) 0;
	os_sched();
	__enable_irq();
}

/*
 * Thread
 */
void thread_init(thread_t* thread, uint8_t priority, void (*handler)(), void* stack, uint32_t stackSize) {
	// Priority must be in range and the priority level must be unused
	OS_ASSERT(thread);
	OS_ASSERT(handler);
	OS_ASSERT(stack);
	OS_ASSERT(priority < DIMENSION(os_threads));
	OS_ASSERT(!os_threads[priority]);

	// Round down the stack top to the 8-byte boundary
	// NOTE: ARM Cortex-M stack grows down from hi -> low memory
	uint32_t* sp = (uint32_t*) ((((uint32_t) stack + stackSize) / 8) * 8);

	*(--sp) = (1 << 24); // xPSR
	*(--sp) = (uint32_t) handler; // PC
	*(--sp) = (uint32_t) os_exit; // LR
	*(--sp) = 0x0000000C; // R12
	*(--sp) = 0x00000003; // R3
	*(--sp) = 0x00000002; // R2
	*(--sp) = 0x00000001; // R1
	*(--sp) = 0x00000000; // R0
	*(--sp) = 0x0000000B; // R11
	*(--sp) = 0x0000000A; // R10
	*(--sp) = 0x00000009; // R9
	*(--sp) = 0x00000008; // R8
	*(--sp) = 0x00000007; // R7
	*(--sp) = 0x00000006; // R6
	*(--sp) = 0x00000005; // R5
	*(--sp) = 0x00000004; // R4
	thread->sp = sp;

	// Pre-fill the unused part of the stack with 0xDEADBEEF
	// Round up the bottom of the stack to the 8-byte boundary
	uint32_t* stackEnd = (uint32_t*) (((((uint32_t) stack - 1) / 8) + 1) * 8);
	for (sp = sp - 1U; sp >= stackEnd; --sp)
		*sp = 0xDEADBEEF;

	// Register the thread with the operating system
	// And make it ready to run
	thread->priority = priority;
	os_threads[thread->priority] = thread;
	if (thread->priority > 0)
		os_ready |= (1 << (thread->priority - 1));
}

/*
 * Semaphore
 */
void semaphore_init(semaphore_t* semaphore, uint32_t max, uint32_t value) {
	OS_ASSERT(semaphore);
	semaphore->max = max;
	semaphore->value = value;
}

void semaphore_wait(semaphore_t* semaphore) {
	OS_ASSERT(semaphore);
	__disable_irq();
	while (semaphore->value == 0) {
		os_yield();
		__disable_irq();
	}
	semaphore->value--;
	__enable_irq();
}

void semaphore_signal(semaphore_t* semaphore) {
	OS_ASSERT(semaphore);
	__disable_irq();
	if (semaphore->value < semaphore->max)
		semaphore->value++;
	__enable_irq();
}

/*
 * Handlers
 */
__attribute__((weak))
void assert_handler(const char* module, int line) {
	rcc_init();
	usart_init(USART1, 115200);
	std_printf("ASSERT AT %s:%d FAILED. HALTING\n", module, line);
	for (;;);
}

void systick_handler(void) {
	os_tick();
	__disable_irq();
	os_sched();
	__enable_irq();
}

__attribute__ ((naked))
void pendsv_handler(void) {
	asm volatile (
		// __disable_irq();
		"  cpsid i\n"
		// if (os_current != (Thread*) 0) {
		"  ldr r1, =os_current\n"
		"  ldr r1, [r1, #0]\n"
		"  cbz r1, pendsv_restore\n"
		//	 push registers r4-r11 on the stack
		"  push {r4-r11}\n"
		//	 os_current->sp = sp;
		"  ldr r1,=os_current\n"
		"  ldr r1,[r1,#0x00]\n"
		"  str sp,[r1,#0x00]\n"
		// }
		"pendsv_restore:\n"
		// sp = os_next->sp;
		"  ldr r1, =os_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr sp, [r1, #0]\n"
		// os_current = os_next;
		"  ldr r1, =os_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr r2, =os_current\n"
		"  str r1, [r2, #0]\n"
		// pop registers r4-r11
		"  pop {r4-r11}\n"
		// __enable_irq();
		"  cpsie i\n"
		// return;
		"  bx lr\n"
	);
}
