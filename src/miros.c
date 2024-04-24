#include "miros.h"

#include <stddef.h>
#include <stdint.h>

#include "stm32.h"

#define OS_MAX_THREADS 32

static thread_t* os_threads[OS_MAX_THREADS];
static thread_t* os_thread_current;
static thread_t* os_thread_next;
static uint32_t os_ticks;

thread_t idle_thread;
uint8_t idle_stack[256] __attribute__ ((aligned(8)));
void idle_main(void) {
	while (true) {}
}

static void os_schedule(void) {
	os_thread_next = os_threads[0];
	uint32_t earliest_absolute_deadline = UINT32_MAX;
	for (uint32_t i = 1; i < OS_MAX_THREADS; i++) {
		thread_t* thread = os_threads[i];
		if (!thread || thread->activation_time > os_ticks || thread->delayed_until > os_ticks)
			continue;
		uint32_t absolute_deadline = thread->activation_time + thread->relative_deadline;
		if (absolute_deadline <= earliest_absolute_deadline) {
			os_thread_next = thread;
			earliest_absolute_deadline = absolute_deadline;
		}
	}
	if (os_thread_next != os_thread_current) {
		if (os_thread_current != NULL) {
#if defined(OS_DEBUG_GPIO)
			gpio_set(GPIOA, os_thread_current->debug_pin, false);
#endif
#if defined(OS_DEBUG_USART)
			usart_write(USART1, 1);
			usart_write(USART1, os_thread_current->debug_pin - 8);
#endif
		}
#if defined(OS_DEBUG_GPIO)
		gpio_set(GPIOA, os_thread_next->debug_pin, true);
#endif
#if defined(OS_DEBUG_USART)
		usart_write(USART1, 0);
		usart_write(USART1, os_thread_next->debug_pin - 8);
#endif

		SCB->icsr |= SCB_ICSR_PENDSVSET;
		asm volatile ("dsb");
	}
}

void os_init(void) {
	gpio_init(GPIOA);
	nvic_set_priority(IRQN_PENDSV, 0xFF);
	idle_thread = (thread_t) {
		.stack_begin = &idle_stack[sizeof(idle_stack)],
		.entry_point = &idle_main,
		.relative_deadline = UINT32_MAX,
		.period = 0,
		.debug_pin = 8
	};
	os_thread_add(&idle_thread);
	os_ticks = 0;
}

void os_thread_add(thread_t* thread) {
	OS_ASSERT(thread);
	thread->stack_pointer = (uint32_t*) thread->stack_begin;
	*(--thread->stack_pointer) = (1 << 24); // xPSR
	*(--thread->stack_pointer) = (uint32_t) thread->entry_point; // PC
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

	thread->activation_time = os_ticks;
	thread->delayed_until = os_ticks;

#if defined(OS_DEBUG_GPIO)
	gpio_configure(GPIOA, thread->debug_pin, GPIO_CR_MODE_OUTPUT_2M, GPIO_CR_CNF_OUTPUT_OPEN_DRAIN);
	gpio_set(GPIOA, thread->debug_pin, false);
#endif

	for (uint32_t i = 0; i < OS_MAX_THREADS; i++) {
		if (os_threads[i] == NULL) {
			os_threads[i] = thread;
			return;
		}
	}

	OS_ERROR();
}

void os_start(void) {
	rcc_init();
	usart_init(USART1, 72e6 / 115200);
	systick_init(72e6 / OS_SECONDS(1));
	nvic_set_priority(IRQN_SYSTICK, 0x00);
	__disable_irq();
	os_schedule();
	__enable_irq();
	OS_ERROR();
}

void os_burn(uint32_t ticks) {
	uint32_t previous = os_ticks;
	while (ticks--) {
		while (os_ticks == previous)
			asm volatile ("nop");
		previous = os_ticks;
	}
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
	if (os_thread_current->period == 0) {
		for (uint32_t i = 1; i < OS_MAX_THREADS; i++) {
			if (os_threads[i] == os_thread_current) {
				os_threads[i] = NULL;
				break;
			}
		}
	} else {
		os_thread_current->activation_time += os_thread_current->period;
	}
	os_schedule();
	__enable_irq();
	asm volatile (
		"  ldr lr, =os_exit\n"
		"  ldr r1, =os_thread_current\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr sp, [r1, #0]\n"
		"  ldr r1, [r1, #8]\n"
		"  bx r1\n"
	);
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
	while (true) {}
}

void nmi_handler(void) {
	while (true) {}
}

void hard_fault_handler(void) {
	while (true) {}
}

void mm_fault_handler(void) {
	while (true) {}
}

void bus_fault_handler(void) {
	while (true) {}
}

void usage_fault_handler(void) {
	while (true) {}
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
		//	 os_thread_current->stack_pointer = sp;
		"  ldr r1, =os_thread_current\n"
		"  ldr r1, [r1, #0]\n"
		"  str sp, [r1, #4]\n"
		// }
		"pendsv_restore:\n"
		// sp = os_thread_next->stack_pointer;
		"  ldr r1, =os_thread_next\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr sp, [r1, #4]\n"
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
