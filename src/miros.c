#include "miros.h"

#include <stddef.h>
#include <stdint.h>

#include "stm32.h"

#define max(a,b) ({ \
	__typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; \
})

#define OS_MAX_THREADS 32
static thread_t* os_threads[OS_MAX_THREADS];
static thread_t* os_thread_current;
static thread_t* os_thread_next;
static uint32_t os_ticks;
static uint32_t os_server_inverse_bandwidth;

#define OS_MAX_APERIODIC_TASKS 64
static struct {
	aperiodic_task_t tasks[OS_MAX_APERIODIC_TASKS];
	uint32_t head;
	uint32_t tail;
} aperiodic_task_queue = {
	.head = 0,
	.tail = 0,
};

bool os_enqueue_aperiodic_task(void (*entry_point)(void), uint32_t computation_time) {
	__disable_irq();

	// If the queue is full, return false
	if ((aperiodic_task_queue.head + 1) % OS_MAX_APERIODIC_TASKS == aperiodic_task_queue.tail)
		return false;

	aperiodic_task_t* aperiodic_task = &aperiodic_task_queue.tasks[aperiodic_task_queue.head];
	aperiodic_task->entry_point = entry_point;

	// Calculate a new absolute deadline by the equation max(t, d_(k-1)) + C/(1-U) where
	//   t is the current time
	//   d_(k-1) is the absolute deadline of the previous aperiodic request
	//   C is the aperiodic task computation time
	//   1/(1-U) is the inverse server bandwidth
	// Start with d_0 = 0
	static uint32_t previous_absolute_deadline = 0;
	aperiodic_task->absolute_deadline = max(os_ticks, previous_absolute_deadline) + computation_time * os_server_inverse_bandwidth;
	previous_absolute_deadline = aperiodic_task->absolute_deadline;

	aperiodic_task_queue.head = (aperiodic_task_queue.head + 1) % OS_MAX_APERIODIC_TASKS;

	__enable_irq();
	return true;
}

static bool os_dequeue_aperiodic_task(aperiodic_task_t* aperiodic_task) {
	__disable_irq();
	// If queue is empty, return false
	if (aperiodic_task_queue.head == aperiodic_task_queue.tail)
		return false;
	*aperiodic_task = aperiodic_task_queue.tasks[aperiodic_task_queue.tail];
	aperiodic_task_queue.tail = (aperiodic_task_queue.tail + 1) % OS_MAX_APERIODIC_TASKS;
	__enable_irq();
	return true;
}

static bool os_peek_aperiodic_task(aperiodic_task_t* aperiodic_task) {
	// If queue is empty, return false
	if (aperiodic_task_queue.head == aperiodic_task_queue.tail)
		return false;
	*aperiodic_task = aperiodic_task_queue.tasks[aperiodic_task_queue.tail];
	return true;
}

static thread_t os_idle_thread;
static uint8_t os_idle_stack[256] __attribute__ ((aligned(8)));
static void os_idle_main(void) {
	while (true) {}
}

static thread_t os_server_thread; 
static uint8_t os_server_stack[256] __attribute__ ((aligned(8)));
static void os_server_main(void) {
	aperiodic_task_t aperiodic_task;
	if (!os_dequeue_aperiodic_task(&aperiodic_task))
		return;
	aperiodic_task.entry_point();
}

static void os_schedule(void) {
	// If there is an unserved aperiodic task and the server is not active, activate it
	aperiodic_task_t aperiodic_task;
	if (os_peek_aperiodic_task(&aperiodic_task) && os_server_thread.activation_time == UINT32_MAX) {
		os_server_thread.relative_deadline = aperiodic_task.absolute_deadline - os_ticks;
		os_server_thread.activation_time = os_ticks;
	}

	// Find the highest-priority thread (smallest absolute deadline)
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

	// Switch to the highest-priority thread
	if (os_thread_next != os_thread_current) {
		// Turn on and off debugging pins
		if (os_thread_current != NULL) {
			#if defined(OS_DEBUG_GPIO)
				gpio_write(GPIOA, os_thread_current->id + 2, false);
			#endif
			#if defined(OS_DEBUG_USART)
				usart_write(USART1, 1);
				usart_write(USART1, os_thread_current->id);
			#endif
		}
		#if defined(OS_DEBUG_GPIO)
			gpio_write(GPIOA, os_thread_current->id + 2, true);
		#endif
		#if defined(OS_DEBUG_USART)
			usart_write(USART1, 0);
			usart_write(USART1, os_thread_current->id);
		#endif

		// Force a PendSV exception
		SCB->icsr |= SCB_ICSR_PENDSVSET;
		asm volatile ("dsb");
	}
}

void os_init(uint32_t server_inverse_bandwidth) {
	#if defined(OS_DEBUG_GPIO)
		gpio_init(GPIOA);
	#endif
	#if defined(OS_DEBUG_USART)
		usart_init(USART1, rcc_get_clock() / 115200);
	#endif

	// Set PendSV to the lowest priority
	nvic_set_priority(IRQN_PENDSV, 0xFF);

	os_ticks = 0;

	os_idle_thread = (thread_t) {
		.stack_begin = &os_idle_stack[sizeof(os_idle_stack)],
		.entry_point = &os_idle_main,
		.relative_deadline = UINT32_MAX,
		.period = UINT32_MAX,
	};
	os_add_thread(&os_idle_thread);

	os_server_inverse_bandwidth = server_inverse_bandwidth;
	os_server_thread = (thread_t) {
		.stack_begin = &os_server_stack[sizeof(os_server_stack)],
		.entry_point = &os_server_main,
		.relative_deadline = UINT32_MAX,
		.period = UINT32_MAX,
	};
	os_add_thread(&os_server_thread);
	os_server_thread.activation_time = UINT32_MAX;
}

void os_add_thread(thread_t* thread) {
	OS_ASSERT(thread);

	// Allocate a slot for this thread
	for (thread->id = 0; thread->id < OS_MAX_THREADS; thread->id++)
		if (os_threads[thread->id] == NULL)
			break;
	OS_ASSERT(thread->id < OS_MAX_THREADS);
	os_threads[thread->id] = thread;

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
		gpio_configure(GPIOA, thread->id + 2, GPIO_CR_MODE_OUTPUT_2M, GPIO_CR_CNF_OUTPUT_PUSH_PULL);
		gpio_write(GPIOA, thread->id + 2, false);
	#endif
}

void os_start(void) {
	__disable_irq();

	// Initialize SysTick such that OS_SECONDS(1) is in fact equals to one second
	// and assign it the highest priority
	systick_init(rcc_get_clock() / OS_SECONDS(1));
	nvic_set_priority(IRQN_SYSTICK, 0x00);

	// Schedule the first thread and jump to it!
	os_schedule();
	__enable_irq();

	OS_ASSERT(false);
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

	// Add the period to the activation time. If it overflows, set it to the highest possible value
	if (__builtin_add_overflow(os_thread_current->activation_time, os_thread_current->period, &os_thread_current->activation_time))
		os_thread_current->activation_time = UINT32_MAX;

	// Schedule the next thread
	os_schedule();
	__enable_irq();

	// Once this thread gets scheduled again, set the lr register to os_exit, and jump to the thread's entry point
	asm volatile (
		"  ldr lr, =os_exit\n"
		"  ldr r1, =os_thread_current\n"
		"  ldr r1, [r1, #0]\n"
		"  ldr sp, [r1, #0]\n"
		"  ldr r1, [r1, #8]\n"
		"  bx r1\n"
	);
}

uint32_t os_current_millis(void) {
	return os_ticks / OS_MILLIS(1);
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
	// NOTE(Tiago): Do not enable IRQs now as part of the NPP.
	// This means that as soon as semaphore_wait() returns, we are
	// in a critical section.
	// __enable_irq();
}

void semaphore_signal(semaphore_t* semaphore) {
	OS_ASSERT(semaphore);
	__disable_irq();
	if (semaphore->current_value < semaphore->maximum_value)
		semaphore->current_value++;
	// NOTE(Tiago): Enable IRQs now as part of the NPP.
	// This means that as soon as semaphore_signal() returns, we are
	// *out* of a critical session.
	// This *disallows* the use of nested critical sections.
	// A simple "fix" would be to remove this call to __enable_irq()
	// and leave this responsibility to the user.
	__enable_irq();
}

/*
 * Handlers
 */
void assert_handler(const char* module, int line) {
	(void) module, (void) line;

	__disable_irq();

	// Initialize the on-board LED (pin C13)
	gpio_init(GPIOC);
	gpio_configure(GPIOC, 13, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_PUSH_PULL);

	// And blink it forever
	bool led_state = false;
	while (true) {
		gpio_write(GPIOC, 13, led_state = !led_state);
		for (volatile int i = 0; i < 5e5; i++) {}
	}
}

void nmi_handler(void) {
	OS_ASSERT(false);
}

void hard_fault_handler(void) {
	OS_ASSERT(false);
}

void mm_fault_handler(void) {
	OS_ASSERT(false);
}

void bus_fault_handler(void) {
	OS_ASSERT(false);
}

void usage_fault_handler(void) {
	OS_ASSERT(false);
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

void systick_handler(void) {
	__disable_irq();
	os_ticks++;
	os_schedule();
	__enable_irq();
}
