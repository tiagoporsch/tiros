.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.section .isr_vector
.word _estack			/* Stack top address */
.word reset_handler		/*  1 Reset */
.word nmi_handler		/*  2 NMI */
.word hard_fault_handler	/*  3 Hard Fault */
.word mm_fault_handler		/*  4 MM Fault */
.word bus_fault_handler		/*  5 Bus Fault */
.word usage_fault_handler	/*  6 Usage Fault */
.word 0				/*  7 RESERVED */
.word 0				/*  8 RESERVED */
.word 0				/*  9 RESERVED*/
.word 0				/* 10 RESERVED */
.word default_handler		/* 11 SV call */
.word default_handler		/* 12 Debug reserved */
.word 0				/* 13 RESERVED */
.word pendsv_handler		/* 14 PendSV */
.word systick_handler		/* 15 SysTick */

default_handler:
	b default_handler

.type reset_handler, %function
reset_handler:
	/* Clear the BSS segment */
	ldr r0, =_sbss
	ldr r1, =_ebss
	mov r2, #0
0:	cmp r0, r1
	it lt
	strlt r2, [r0], #4
	blt 0b

	/* Jump to main */
	bl main
	b .
