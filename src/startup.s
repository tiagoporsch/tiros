.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.section .isr_vector
.word _estack			/* Stack top address */
.word reset_handler		/* -15 Reset */
.word nmi_handler		/* -14 NMI */
.word hard_fault_handler	/* -13 Hard Fault */
.word mm_fault_handler		/* -12 MM Fault */
.word bus_fault_handler		/* -11 Bus Fault */
.word usage_fault_handler	/* -10 Usage Fault */
.word 0				/* - 9 RESERVED */
.word 0				/* - 8 RESERVED */
.word 0				/* - 7 RESERVED*/
.word 0				/* - 6 RESERVED */
.word default_handler		/* - 5 SV call */
.word default_handler		/* - 4 Debug reserved */
.word 0				/* - 3 RESERVED */
.word pendsv_handler		/* - 2 PendSV */
.word systick_handler		/* - 1 SysTick */
.word default_handler		/*   0 WWDG */
.word default_handler		/*   1 PVD */
.word default_handler		/*   2 TAMPER */
.word default_handler		/*   3 RTC */
.word default_handler		/*   4 FLASH */
.word default_handler		/*   5 RCC */
.word default_handler		/*   6 EXTI0 */
.word default_handler		/*   7 EXTI1 */
.word default_handler		/*   8 EXTI2 */
.word default_handler		/*   9 EXTI3 */
.word default_handler		/*  10 EXTI4 */
.word default_handler		/*  11 DMA1_1 */
.word default_handler		/*  12 DMA1_1 */
.word default_handler		/*  13 DMA1_1 */
.word default_handler		/*  14 DMA1_1 */
.word default_handler		/*  15 DMA1_1 */
.word default_handler		/*  16 DMA1_1 */
.word default_handler		/*  17 DMA1_1 */
.word default_handler		/*  18 ADC1_2 */
.word default_handler		/*  19 CAN1_TX */
.word default_handler		/*  20 CAN1_RX0 */
.word default_handler		/*  21 CAN1_RX1 */
.word default_handler		/*  22 CAN1_SCE */
.word exti9_5_handler		/*  23 EXTI9_5 */

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

	/* Configure the clock */
	bl rcc_init

	/* Jump to main */
	bl main
	b .
