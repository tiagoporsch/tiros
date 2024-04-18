#include "miros.h"

#include <stddef.h>
#include <stdint.h>

#include "std.h"
#include "stm32.h"

#define OS_MAX_THREADS 32
static thread_t* os_threads[OS_MAX_THREADS];
static thread_t* os_thread_current;
static thread_t* os_thread_next;
static uint32_t os_ticks;

static uint32_t idle_stack[128];
static thread_t idle_thread;
static void idle_main(void) {
	while (true) {}
}

static void os_schedule(void) {
	os_thread_next = os_threads[0];
	uint32_t earliest_absolute_deadline = UINT32_MAX;
	for (size_t i = 1; i < OS_MAX_THREADS; i++) {
		thread_t* thread = os_threads[i];
		if (!thread || thread->activation_time > os_ticks || thread->delayed_until > os_ticks)
			continue;
		uint32_t absolute_deadline = thread->activation_time + thread->relative_deadline;
		if (absolute_deadline < earliest_absolute_deadline) {
			os_thread_next = thread;
			earliest_absolute_deadline = absolute_deadline;
		}
	}
	if (os_thread_next != os_thread_current) {
		SCB->icsr |= SCB_ICSR_PENDSVSET;
		asm volatile ("dsb");
	}
}

void os_init(void) {
	nvic_set_priority(IRQN_PENDSV, 0xFF);
	thread_init(&idle_thread, idle_stack, sizeof(idle_stack), &idle_main, UINT32_MAX, UINT32_MAX);
	os_ticks = 0;
}

void os_start(void) {
	rcc_init();
	systick_init(72e6 / OS_SECONDS(1));
	nvic_set_priority(IRQN_SYSTICK, 0x00);
	__disable_irq();
	os_schedule();
	__enable_irq();
	OS_ERROR();
}

void os_delay(uint32_t ticks) {
	__disable_irq();
	os_thread_current->delayed_until = os_ticks + ticks;
	os_schedule();
	__enable_irq();
}

void os_yield(void) {
	os_delay(1);
}

void os_exit(void) {
	__disable_irq();
	os_thread_current->activation_time += os_thread_current->period;
	os_schedule();
	__enable_irq();
	asm volatile (
		"  ldr lr, =os_exit\n"
		"  ldr r1, =os_thread_current\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr r1, [r1, #4]\n"
		"  bx r1\n"
	);
}

/*
 * Thread
 */
void thread_init(thread_t* thread, void* stack, uint32_t stack_size, void (*entry_point)(), uint32_t relative_deadline, uint32_t period) {
	OS_ASSERT(thread);
	OS_ASSERT(stack);
	OS_ASSERT(stack_size >= 32 * sizeof(int));
	OS_ASSERT(entry_point);

	thread->stack_pointer = (uint32_t*) ((((uint32_t) stack + stack_size) / 8) * 8);
	*(--thread->stack_pointer) = (1 << 24); // xPSR
	*(--thread->stack_pointer) = (uint32_t) entry_point; // PC
	*(--thread->stack_pointer) = (uint32_t) os_exit; // LR
	*(--thread->stack_pointer) = 0x0000000C; // R12
	*(--thread->stack_pointer) = 0x00000003; // R3
	*(--thread->stack_pointer) = 0x00000002; // R2
	*(--thread->stack_pointer) = 0x00000001; // R1
	*(--thread->stack_pointer) = 0x00000000; // R0
	*(--thread->stack_pointer) = 0x0000000B; // R11
	*(--thread->stack_pointer) = 0x0000000A; // R10
	*(--thread->stack_pointer) = 0x00000009; // R9
	*(--thread->stack_pointer) = 0x00000008; // R8
	*(--thread->stack_pointer) = 0x00000007; // R7
	*(--thread->stack_pointer) = 0x00000006; // R6
	*(--thread->stack_pointer) = 0x00000005; // R5
	*(--thread->stack_pointer) = 0x00000004; // R4

	thread->entry_point = entry_point;
	thread->relative_deadline = relative_deadline;
	thread->period = period;
	thread->activation_time = os_ticks;
	thread->delayed_until = os_ticks;

	for (size_t i = 0; i < OS_MAX_THREADS; i++) {
		if (os_threads[i] == NULL) {
			os_threads[i] = thread;
			return;
		}
	}

	OS_ERROR();
}

/*
 * Semaphore
 */
void semaphore_init(semaphore_t* semaphore, uint32_t maximum_value, uint32_t starting_value) {
	OS_ASSERT(semaphore);
	semaphore->maximum_value = maximum_value;
	semaphore->current_value = starting_value;
}

void semaphore_wait(semaphore_t* semaphore) {
	OS_ASSERT(semaphore);
	__disable_irq();
	while (semaphore->current_value == 0) {
		os_yield();
		__disable_irq();
	}
	semaphore->current_value--;
	__enable_irq();
}

void semaphore_signal(semaphore_t* semaphore) {
	OS_ASSERT(semaphore);
	__disable_irq();
	if (semaphore->current_value < semaphore->maximum_value)
		semaphore->current_value++;
	__enable_irq();
}

/*
 * Handlers
 */
void assert_handler(const char* module, int line) {
	std_printf("Assert %s:%d!\n", module, line);
	while (true);
}

void nmi_handler(void) {
	std_printf("NMI!\n");
	while (true);
}

void hard_fault_handler(void) {
	std_printf("Hard Fault!\n");
	while (true);
}

void mm_fault_handler(void) {
	std_printf("MM Fault!\n");
	while (true);
}

void bus_fault_handler(void) {
	std_printf("Bus Fault!\n");
	while (true);
}

void usage_fault_handler(void) {
	std_printf("Usage Fault!\n");
	while (true);
}

void systick_handler(void) {
	__disable_irq();
	os_ticks++;
	os_schedule();
	__enable_irq();
}

__attribute__ ((naked))
void pendsv_handler(void) {
	asm volatile (
		// __disable_irq();
		"  cpsid i\n"
		// if (os_thread_current != NULL) {
		"  ldr r1, =os_thread_current\n"
		"  ldr r1, [r1, #0]\n"
		"  cbz r1, pendsv_restore\n"
		//	 push registers r4 to r11
		"  push {r4-r11}\n"
		//	 os_thread_current->stack_pointer = stack_pointer;
		"  ldr r1, =os_thread_current\n"
		"  ldr r1, [r1, #0]\n"
		"  str sp, [r1, #0]\n"
		// }
		"pendsv_restore:\n"
		// stack_pointer = os_thread_next->stack_pointer;
		"  ldr r1, =os_thread_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr sp, [r1, #0]\n"
		// os_thread_current = os_thread_next;
		"  ldr r1, =os_thread_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr r2, =os_thread_current\n"
		"  str r1, [r2, #0]\n"
		// pop registers r4 to r11
		"  pop {r4-r11}\n"
		// __enable_irq();
		"  cpsie i\n"
		// return;
		"  bx lr\n"
	);
}
