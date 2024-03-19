/****************************************************************************
* MInimal Real-time Operating System (MiROS), GNU-ARM port.
* version 1.26 (matching lesson 26, see https://youtu.be/kLxxXNCrY60)
*
* This software is a teaching aid to illustrate the concepts underlying
* a Real-Time Operating System (RTOS). The main goal of the software is
* simplicity and clear presentation of the concepts, but without dealing
* with various corner cases, portability, or error handling. For these
* reasons, the software is generally NOT intended or recommended for use
* in commercial applications.
*
* Copyright (C) 2018 Miro Samek. All Rights Reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*
* Git repo:
* https://github.com/QuantumLeaps/MiROS
****************************************************************************/
#include "miros.h"

#include <stdint.h>

#include "qassert.h"

Q_DEFINE_THIS_FILE

#define LOG2(x) (32U - __builtin_clz(x))

static OSThread* volatile os_curr; // pointer to the current thread
static OSThread* volatile os_next; // pointer to the next thread to run
static OSThread *os_threads[32 + 1]; // array of threads started so far
static uint32_t os_readySet; // bitmask of threads that are ready to run
static uint32_t os_delayedSet; // bitmask of threads that are delayed

static uint32_t idle_stack[40];
static OSThread idle_thread;
static void idle_main() {
    for (;;) {
        os_onIdle();
    }
}

void os_init(void) {
    // Set the PendSV interrupt priority to the lowest level (0xFF)
    (*(uint32_t volatile*) 0xE000ED20) |= (0xFFU << 16);

    // Start the idle thread
    os_createThread(&idle_thread, 0U, &idle_main, idle_stack, sizeof(idle_stack));
}

void os_sched(void) {
    /* choose the next thread to execute... */
    OSThread *next;
    if (os_readySet == 0U) { /* idle condition? */
        next = os_threads[0]; /* the idle thread */
    } else {
        next = os_threads[LOG2(os_readySet)];
        Q_ASSERT(next != (OSThread *)0);
    }

    /* trigger PendSV, if needed */
    if (next != os_curr) {
        os_next = next;
        (*(uint32_t volatile*) 0xE000ED04) |= (1UL << 28U);
        __asm volatile ("dsb");
    }
}

void os_run(void) {
    os_onStartup();

    __asm volatile ("cpsid i");
    os_sched();
    __asm volatile ("cpsie i");

    Q_ERROR();
}

void os_tick(void) {
    uint32_t workingSet = os_delayedSet;
    while (workingSet != 0U) {
        OSThread *t = os_threads[LOG2(workingSet)];
        uint32_t bit;
        Q_ASSERT((t != (OSThread *)0) && (t->timeout != 0U));

        bit = (1U << (t->prio - 1U));
        --t->timeout;
        if (t->timeout == 0U) {
            os_readySet   |= bit;  /* insert to set */
            os_delayedSet &= ~bit; /* remove from set */
        }
        workingSet &= ~bit; /* remove from working set */
    }
}

void os_delay(uint32_t ticks) {
    uint32_t bit;
    __asm volatile ("cpsid i");

    /* never call OS_delay from the idleThread */
    Q_REQUIRE(os_curr != os_threads[0]);

    os_curr->timeout = ticks;
    bit = (1U << (os_curr->prio - 1U));
    os_readySet &= ~bit;
    os_delayedSet |= bit;
    os_sched();
    __asm volatile ("cpsie i");
}

void os_createThread(OSThread *me, uint8_t prio, OSThreadHandler threadHandler, void *stkSto, uint32_t stkSize) {
    /* round down the stack top to the 8-byte boundary
    * NOTE: ARM Cortex-M stack grows down from hi -> low memory
    */
    uint32_t *sp = (uint32_t *)((((uint32_t) stkSto + stkSize) / 8) * 8);
    uint32_t *stk_limit;

    /* priority must be in ragne
    * and the priority level must be unused
    */
    Q_REQUIRE((prio < Q_DIM(os_threads)) && (os_threads[prio] == (OSThread *)0));

    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t) threadHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */

    /* save the top of the stack in the thread's attibute */
    me->sp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t) stkSto - 1U) / 8) + 1U) * 8);

    /* pre-fill the unused part of the stack with 0xDEADBEEF */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEFU;
    }

    /* register the thread with the OS */
    os_threads[prio] = me;
    me->prio = prio;
    /* make the thread ready to run */
    if (prio > 0U) {
        os_readySet |= (1U << (prio - 1U));
    }
}

__attribute__ ((naked, optimize("-fno-stack-protector")))
void pendsv_handler(void) {
__asm volatile (

    /* __disable_irq(); */
    "  CPSID         I                 \n"

    /* if (OS_curr != (OSThread *)0) { */
    "  LDR           r1,=os_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CBZ           r1,pendsv_restore \n"

    /*     push registers r4-r11 on the stack */
    "  PUSH          {r4-r11}          \n"

    /*     OS_curr->sp = sp; */
    "  LDR           r1,=os_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  STR           sp,[r1,#0x00]     \n"
    /* } */

    "pendsv_restore:                   \n"
    /* sp = OS_next->sp; */
    "  LDR           r1,=os_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           sp,[r1,#0x00]     \n"

    /* OS_curr = OS_next; */
    "  LDR           r1,=os_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=os_curr       \n"
    "  STR           r1,[r2,#0x00]     \n"

    /* pop registers r4-r11 */
    "  POP           {r4-r11}          \n"

    /* __enable_irq(); */
    "  CPSIE         I                 \n"

    /* return to the next thread */
    "  BX            lr                \n"
    );
}
