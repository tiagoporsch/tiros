#pragma once

#include <stdint.h>

typedef struct {
    void* sp; // stack pointer
    uint32_t timeout; // timeout delay down-counter
    uint8_t priority; // thread priority
} OSThread;

typedef void (*OSThreadHandler)();

void os_init(void);
void os_run(void);

void os_sched(void);
void os_tick(void);
void os_run(void);

void os_yield(void);
void os_delay(uint32_t ticks);

void os_createThread(OSThread* thread, uint8_t priority, void (*handler)(), void* stack, uint32_t stack_size);
