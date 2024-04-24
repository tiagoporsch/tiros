#pragma once

#include <stdint.h>

/*
 * Assert
 */
#define OS_ASSERT(test) ((test) ? (void) 0 : assert_handler(__FILE__, __LINE__))
#define OS_ERROR() assert_handler(__FILE__, __LINE__)

void assert_handler(const char* const file, int line);

/*
 * Thread
 */
typedef struct {
	// These *must* be the first three members of this struct, in *this* order.
	// If they are to be moved around, make sure to update the offsets in the
	// os_exit() and pendsv_handler() functions.
	void* stack_begin;
	uint32_t* stack_pointer;
	void (*entry_point)(void);

	uint32_t relative_deadline;
	uint32_t period;

	uint32_t activation_time;
	uint32_t delayed_until;

	uint8_t debug_pin;
} thread_t;

/*
 * Operating system
 */
#define OS_SECONDS(s) (s * 100)

void os_init(void);
void os_thread_add(thread_t* thread);
void os_start(void);

void os_burn(uint32_t ticks);
void os_delay(uint32_t ticks);
void os_yield(void);
void os_exit(void);

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
