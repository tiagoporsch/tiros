#pragma once

#include <stdint.h>

typedef struct {
    void *sp; // stack pointer
    uint32_t timeout; // yimeout delay down-counter
    uint8_t prio; // thread priority
} OSThread;

typedef void (*OSThreadHandler)();

// callback to configure and start interrupts
void os_onStartup(void);

// callback to handle the idle condition
void os_onIdle(void);

void os_init();

// this function must be called with interrupts disabled
void os_sched(void);

// transfer control to the RTOS to run the threads
void os_run(void);

// blocking delay
void os_delay(uint32_t ticks);

// process all timeouts
void os_tick(void);

void os_createThread(OSThread* thread, uint8_t priority, OSThreadHandler handler, void* stack, uint32_t stack_size);
