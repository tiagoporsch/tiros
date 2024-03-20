#pragma once

#include <stdint.h>

#define OS_ONE_SECOND 100

void os_init(void);
void os_run(void);

void os_sched(void);
void os_tick(void);

void os_yield(void);
void os_delay(uint32_t ticks);

/*
 * Thread
 */
typedef struct {
	void* sp; // stack pointer
	uint32_t timeout; // timeout delay down-counter
	uint8_t priority; // thread priority
} Thread;

void thread_init(Thread* thread, uint8_t priority, void (*handler)(), void* stack, uint32_t stack_size);

/*
 * Semaphore
 */
typedef struct {
	volatile int32_t value;
} Semaphore;

void semaphore_init(Semaphore* semaphore, int32_t value);
void semaphore_wait(Semaphore* semaphore);
void semaphore_signal(Semaphore* semaphore);
