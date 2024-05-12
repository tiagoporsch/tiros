.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.section .isr_vector
	.word _estack			/* Stack top address */
	.word reset_handler		/* -15 Reset */
	.word nmi_handler		/* -14 NMI */
	.word hard_fault_handler	/* -13 HardFault */
	.word mm_fault_handler		/* -12 MemManage */
	.word bus_fault_handler		/* -11 BusFault */
	.word usage_fault_handler	/* -10 UsageFault */
	.word 0				/*  -9 */
	.word 0				/*  -8 */
	.word 0				/*  -7 */
	.word 0				/*  -6 */
	.word 0				/*  -5 SVCall */
	.word 0				/*  -4 Debug Monitor */
	.word 0				/*  -3 */
	.word pendsv_handler		/*  -2 PendSV */
	.word systick_handler		/*  -1 SysTick */
	.word 0				/*   0 WWDG */
	.word 0				/*   1 PVD */
	.word 0				/*   2 TAMPER */
	.word 0				/*   3 RTC */
	.word 0				/*   4 FLASH */
	.word 0				/*   5 RCC */
	.word 0				/*   6 EXTI0 */
	.word 0				/*   7 EXTI1 */
	.word 0				/*   8 EXTI2 */
	.word 0				/*   9 EXTI3 */
	.word 0				/*  10 EXTI4 */
	.word 0				/*  11 DMA1_Channel1 */
	.word 0				/*  12 DMA1_Channel1 */
	.word 0				/*  13 DMA1_Channel1 */
	.word 0				/*  14 DMA1_Channel1 */
	.word 0				/*  15 DMA1_Channel1 */
	.word 0				/*  16 DMA1_Channel1 */
	.word 0				/*  17 DMA1_Channel1 */
	.word 0				/*  18 ADC1_2 */
	.word 0				/*  19 CAN1_TX */
	.word 0				/*  20 CAN1_RX0 */
	.word 0				/*  21 CAN1_RX1 */
	.word 0				/*  22 CAN1_SCE */
	.word exti9_5_handler		/*  23 EXTI9_5 */

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
