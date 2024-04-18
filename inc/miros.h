#pragma once

#include <stdint.h>

/*
 * Operating system
 */
#define OS_MILLISECONDS(ms) (ms)
#define OS_SECONDS(s) (s * OS_MILLISECONDS(1000))

void os_init(void);
void os_start(void);

void os_delay(uint32_t ticks);
void os_yield(void);
void os_exit(void);

/*
 * Assert
 */
#define OS_ASSERT(test) ((test) ? (void) 0 : assert_handler(__FILE__, __LINE__))
#define OS_ERROR() assert_handler(__FILE__, __LINE__)

void assert_handler(const char* const file, int line);

/*
 * Semaphore
 */
typedef struct {
	uint32_t maximum_value;
	uint32_t current_value;
} semaphore_t;

void semaphore_init(semaphore_t* semaphore, uint32_t maximum_value, uint32_t starting_value);
void semaphore_wait(semaphore_t* semaphore);
void semaphore_signal(semaphore_t* semaphore);

/*
 * Thread
 */
typedef struct {
	// These *must* be the first two members of this struct, in *this* order.
	// If they are to be moved around, make sure to update the offsets in the
	// os_exit() and pendsv_handler() functions.
	uint32_t* stack_pointer;
	void (*entry_point)();

	uint32_t relative_deadline;
	uint32_t period;

	uint32_t activation_time;
	uint32_t delayed_until;
} thread_t;

void thread_init(thread_t* thread, void* stack, uint32_t stack_size, void (*entry_point)(), uint32_t relative_deadline, uint32_t period);
