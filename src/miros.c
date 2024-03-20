#include "miros.h"

#include <stdint.h>

#include "cm3.h"
#include "qassert.h"

Q_DEFINE_THIS_FILE

#define LOG2(x) (32U - __builtin_clz(x))

static Thread* volatile os_curr; // pointer to the current thread
static Thread* volatile os_next; // pointer to the next thread to run
static Thread* os_threads[32 + 1]; // array of threads started so far
static uint32_t os_readySet; // bitmask of threads that are ready to run
static uint32_t os_delayedSet; // bitmask of threads that are delayed

static uint32_t idle_stack[40];
static Thread idle_thread;
static void idle_main(void) {
	for (;;);
}

void os_init(void) {
	// set the PendSV interrupt priority to the lowest level (0xFF)
	(*(uint32_t volatile*) 0xE000ED20) |= (0xFFU << 16);

	// start the idle thread
	thread_init(&idle_thread, 0U, &idle_main, idle_stack, sizeof(idle_stack));
}

void os_run(void) {
	rcc_init();
	uart_init(UART1, 115200);
	systick_init(RCC_SYS_CLOCK / OS_ONE_SECOND);
	nvic_setPriotity(-1, 0x00U);

	asm volatile ("cpsid i" : : : "memory");
	os_sched();
	asm volatile ("cpsie i" : : : "memory");
	Q_ERROR();
}

void os_sched(void) {
	Thread* next = os_threads[LOG2(os_readySet)];
	if (next != os_curr) {
		os_next = next;
		(*(uint32_t volatile*) 0xE000ED04) |= (1UL << 28U);
		asm volatile ("dsb" : : : "memory");
	}
}

void os_tick(void) {
	uint32_t workingSet = os_delayedSet;
	while (workingSet != 0U) {
		Thread* thread = os_threads[LOG2(workingSet)];
		Q_ASSERT((thread != (Thread*) 0) && (thread->timeout != 0));

		uint32_t bit = (1 << (thread->priority - 1));
		if (--thread->timeout == 0U) {
			os_readySet |= bit;
			os_delayedSet &= ~bit;
		}
		workingSet &= ~bit;
	}
}

void os_yield(void) {
	uint32_t bit = (1 << (os_curr->priority - 1));
	Thread* next = os_threads[LOG2(os_readySet & ~bit)];
	if (next != os_curr) {
		os_next = next;
		(*(uint32_t volatile*) 0xE000ED04) |= (1UL << 28U);
		asm volatile ("dsb" : : : "memory");
	}
}

void os_delay(uint32_t ticks) {
	asm volatile ("cpsid i" : : : "memory");

	// never call OS_delay from the idleThread
	Q_REQUIRE(os_curr != os_threads[0]);

	os_curr->timeout = ticks;
	uint32_t bit = (1U << (os_curr->priority - 1U));
	os_readySet &= ~bit;
	os_delayedSet |= bit;
	os_sched();
	asm volatile ("cpsie i" : : : "memory");
}

/*
 * Thread
 */
void thread_init(Thread* thread, uint8_t priority, void (*handler)(), void* stack, uint32_t stackSize) {
	// Priority must be in range and the priority level must be unused
	Q_ASSERT((priority < Q_DIM(os_threads)) && (os_threads[priority] == (Thread*) 0));

	// Round down the stack top to the 8-byte boundary
	// NOTE: ARM Cortex-M stack grows down from hi -> low memory
	uint32_t* sp = (uint32_t*) ((((uint32_t) stack + stackSize) / 8) * 8);

	*(--sp) = (1U << 24); // xPSR
	*(--sp) = (uint32_t) handler; // PC
	*(--sp) = 0x0000000EU; // LR
	*(--sp) = 0x0000000CU; // R12
	*(--sp) = 0x00000003U; // R3
	*(--sp) = 0x00000002U; // R2
	*(--sp) = 0x00000001U; // R1
	*(--sp) = 0x00000000U; // R0
	*(--sp) = 0x0000000BU; // R11
	*(--sp) = 0x0000000AU; // R10
	*(--sp) = 0x00000009U; // R9
	*(--sp) = 0x00000008U; // R8
	*(--sp) = 0x00000007U; // R7
	*(--sp) = 0x00000006U; // R6
	*(--sp) = 0x00000005U; // R5
	*(--sp) = 0x00000004U; // R4
	thread->sp = sp;

	// pre-fill the unused part of the stack with 0xDEADBEEF
	// round up the bottom of the stack to the 8-byte boundary
	uint32_t* stackEnd = (uint32_t*) (((((uint32_t) stack - 1U) / 8) + 1U) * 8);
	for (sp = sp - 1U; sp >= stackEnd; --sp)
		*sp = 0xDEADBEEFU;

	// register the thread with the operating system
	// and make it ready to run
	thread->priority = priority;
	os_threads[thread->priority] = thread;
	if (thread->priority > 0U)
		os_readySet |= (1U << (thread->priority - 1U));
}

/*
 * Semaphore
 */
void semaphore_init(Semaphore* semaphore, int32_t value) {
	semaphore->value = value;
}

void semaphore_wait(Semaphore* semaphore) {
	for (;;) {
		asm volatile ("cpsid i" : : : "memory");
		if (semaphore->value > 0) {
			semaphore->value--;
			asm volatile ("cpsie i" : : : "memory");
			return;
		} else {
			asm volatile ("cpsie i" : : : "memory");
			os_yield();
		}
	}
}

void semaphore_signal(Semaphore* semaphore) {
	asm volatile ("cpsid i" : : : "memory");
	semaphore->value++;
	asm volatile ("cpsie i" : : : "memory");
}

/*
 * Handlers
 */
void assert_handler(const char* module, int line) {
	uart_printf(UART1, "ASSERT FAILED %s:%d\n", module, line);
	for (;;);
}

void systick_handler(void) {
	os_tick();
	asm volatile ("cpsid i" : : : "memory");
	os_sched();
	asm volatile ("cpsie i" : : : "memory");
}

__attribute__ ((naked)) void pendsv_handler(void) {
	asm volatile (
		// __disable_irq();
		"  cpsid i\n"
		// if (os_curr != (OSThread*) 0) {
		"  ldr r1, =os_curr\n"
		"  ldr r1, [r1, #0]\n"
		"  cbz r1, pendsv_restore\n"
		//	 push registers r4-r11 on the stack
		"  push {r4-r11}\n"
		//	 os_curr->sp = sp;
		"  ldr r1,=os_curr\n"
		"  ldr r1,[r1,#0x00]\n"
		"  str sp,[r1,#0x00]\n"
		// }
		"pendsv_restore:\n"
		// sp = os_next->sp;
		"  ldr r1, =os_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr sp, [r1, #0]\n"
		// os_curr = os_next;
		"  ldr r1, =os_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr r2, =os_curr\n"
		"  str r1, [r2, #0]\n"
		// pop registers r4-r11
		"  pop {r4-r11}\n"
		// __enable_irq();
		"  cpsie i\n"
		// return;
		"  bx lr\n"
	);
}
