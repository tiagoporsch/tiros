.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.word   0x20005000      /* stack top address */
.word   _reset          /* 1 Reset */
.word   spin            /* 2 NMI */
.word   spin            /* 3 Hard Fault */
.word   spin            /* 4 MM Fault */
.word   spin            /* 5 Bus Fault */
.word   spin            /* 6 Usage Fault */
.word   spin            /* 7 RESERVED */
.word   spin            /* 8 RESERVED */
.word   spin            /* 9 RESERVED*/
.word   spin            /* 10 RESERVED */
.word   spin            /* 11 SV call */
.word   spin            /* 12 Debug reserved */
.word   spin            /* 13 RESERVED */
.word   pendsv_handler  /* 14 PendSV */
.word   systick_handler /* 15 SysTick */
.word   spin            /* 16 IRQ0 */
.word   spin            /* 17 IRQ1 */
.word   spin            /* 18 IRQ2 */
.word   spin            /* 19 ...   */

spin:
	b spin

.thumb_func
_reset:
	/* Zero-fill the BSS segment */
	ldr r2, =_sbss
	ldr r4, =_ebss
	movs r3, #0
	b loop_fill_zero_bss
fill_zero_bss:
	str r3, [r2]
	adds r2, r2, #4
loop_fill_zero_bss:
	cmp r2, r4
	bcc fill_zero_bss

	/* Jump to main */
	bl main
	b .
