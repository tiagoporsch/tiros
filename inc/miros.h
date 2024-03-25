#pragma once

#include <stdint.h>

/*
 * Operating system
 */
#define OS_ONE_SECOND 100

void os_init(void);
void os_run(void);

void os_yield(void);
void os_delay(uint32_t ticks);
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
typedef uint32_t semaphore_t;

void semaphore_init(semaphore_t* semaphore, uint32_t value);
void semaphore_wait(semaphore_t* semaphore);
void semaphore_signal(semaphore_t* semaphore);

/*
 * Thread
 */
typedef struct {
	void* sp; // stack pointer
	uint32_t timeout; // timeout delay down-counter
	uint8_t priority; // thread priority
} thread_t;

void thread_init(thread_t* thread, uint8_t priority, void (*handler)(), void* stack, uint32_t stack_size);

