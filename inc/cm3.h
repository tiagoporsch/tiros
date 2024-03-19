#pragma once

#include <stdint.h>

/*
 * Nested Vectored Interrupt Controller (NVIC)
 */
struct nvic {
	volatile uint32_t iser[8]; // Interrupt Set Enable Register
	volatile uint32_t reserved0[24];
	volatile uint32_t icer[8]; // Interruppt Clear Enable Register
	volatile uint32_t reserved1[24];
	volatile uint32_t ispr[8]; // Interrupt Set Pending Register
	volatile uint32_t reserved2[24];
	volatile uint32_t icpr[8]; // Interrupt Clear Pending Register
	volatile uint32_t reserved3[24];
	volatile uint32_t iabr[8]; // Interrupt Active Bit Register
	volatile uint32_t reserved4[24];
	volatile uint8_t ip[240]; // Interrupt Priority Register
	volatile uint32_t reserved5[24];
	volatile uint32_t stir; // Software Trigger Interrupt Register
};

#define NVIC_PRIO_BITS 4

void nvic_setPriotity(int irq, uint32_t priority);

/*
 * System Control Block (SCB)
 */
struct scb {
	volatile uint32_t cpuid; // CPUID Base Register
	volatile uint32_t icsr; // Interrupt Control and State Register
	volatile uint32_t vtor; // Vector Table Offset Register
	volatile uint32_t aircr; // Application Interrupt and Reset Control Register
	volatile uint32_t scr; // System Control Register
	volatile uint32_t ccr; // Configuration Control Register
	volatile uint8_t shp[12]; // System Handlers Priority Registers
};

#define SCB ((struct scb*) 0xE000ED00)

/*
 * SYSTICK
 */
struct systick {
	volatile uint32_t csr; // Control and Status Register
	volatile uint32_t load; // Reload Value Register
	volatile uint32_t val; // Current Value Register
	volatile uint32_t calib; // Calibration Register
};

#define SYSTICK ((struct systick*) 0xE000E010)

#define	SYSTICK_ENABLE (1 << 0)
#define	SYSTICK_TICKINT (1 << 1)
#define	SYSTICK_CLKSOURCE (1 << 2)

extern void systick_handler(void);
void systick_init(uint32_t ticks);

/*
 * Reset and Clock Control (RCC)
 */
struct rcc {
	volatile uint32_t cr; // clock control
	volatile uint32_t cfgr; // clock config
	volatile uint32_t cir; // clock interrupt
	volatile uint32_t apb2; // peripheral reset
	volatile uint32_t apb1; // peripheral reset
	volatile uint32_t ape3; // peripheral enable
	volatile uint32_t ape2; // peripheral enable
	volatile uint32_t ape1; // peripheral enable
	volatile uint32_t bdcr; // xx
	volatile uint32_t csr; // xx
};

#define RCC ((struct rcc*) 0x40021000)
#define RCC_SYS_CLOCK 72000000

#define RCC_CR_HSION (1 << 0)
#define RCC_CR_HSITRIM (1 << 7)
#define RCC_CR_HSEON (1 << 16)
#define RCC_CR_PLLON (1 << 24)
#define RCC_CR_PLLRDY (1 << 25)

#define RCC_CFGR_SW_HSI 0x00
#define RCC_CFGR_SW_HSE 0x01
#define RCC_CFGR_SW_PLL 0X02
#define RCC_CFGR_PPRE1_2 (4 << 8)
#define RCC_CFGR_PLLSRC 0x10000
#define RCC_CFGR_PLLMULL9 (7 << 18)

// APE2
#define RCC_GPIOA_ENABLE 0x04
#define RCC_GPIOB_ENABLE 0x08
#define RCC_GPIOC_ENABLE 0x10
#define RCC_TIMER1_ENABLE 0x0800
#define RCC_UART1_ENABLE 0x4000

// APE1
#define RCC_TIMER2_ENABLE 0x0001
#define RCC_TIMER3_ENABLE 0x0002
#define RCC_TIMER4_ENABLE 0x0004
#define RCC_UART2_ENABLE 0x20000
#define RCC_UART3_ENABLE 0x40000

void rcc_init(void);

/*
 * GPIO
 */
struct gpio {
	volatile unsigned long cr[2];
	volatile unsigned long idr;
	volatile unsigned long odr;
	volatile unsigned int bsrrl;
	volatile unsigned int bsrrh;
	volatile unsigned long brr;
	volatile unsigned long lock;
};

#define GPIOA ((struct gpio*) 0x40010800)
#define GPIOB ((struct gpio*) 0x40010C00)
#define GPIOC ((struct gpio*) 0x40011000)

#define GPIO_MODE_INPUT_ANALOG 0
#define GPIO_MODE_INPUT_FLOAT 4
#define GPIO_MODE_INPUT_PUPD 8

#define GPIO_MODE_OUTPUT_10M 1
#define GPIO_MODE_OUTPUT_2M 2
#define GPIO_MODE_OUTPUT_50M 3

#define GPIO_MODE_OUTPUT_PUSH_PULL 0
#define GPIO_MODE_OUTPUT_OPEN_DRAIN 4

#define GPIO_MODE_ALT_PUSH_PULL 8
#define GPIO_MODE_ALT_OPEN_DRAIN 12

void gpio_init(struct gpio* gpio);
void gpio_mode(struct gpio* gpio, int pin, int mode);
void gpio_on(struct gpio* gpio, int pin);
void gpio_off(struct gpio* gpio, int pin);

/*
 * UART
 */
struct uart {
	volatile uint32_t status;
	volatile uint32_t data;
	volatile uint32_t baud;
	volatile uint32_t cr1;
	volatile uint32_t cr2;
	volatile uint32_t cr3;
	volatile uint32_t gtp; // guard time and prescaler
};

#define UART1 ((struct uart*) 0x40013800)
#define UART2 ((struct uart*) 0x40004400)
#define UART3 ((struct uart*) 0x40004800)

#define UART_PE 0x0001
#define UART_FE 0x0002
#define	UART_NE 0x0004
#define	UART_OVER 0x0008
#define	UART_IDLE 0x0010
#define	UART_RXNE 0x0020 /* Receiver not empty */
#define	UART_TC 0x0040 /* Transmission complete */
#define	UART_TXE 0x0080 /* Transmitter empty */
#define	UART_BREAK 0x0100
#define	UART_CTS 0x0200

void uart_init(struct uart* uart, int baud);
void uart_putc(struct uart* uart, int c);
void uart_puts(struct uart* uart, const char* s);
void uart_printf(struct uart* uart, const char* format, ...);
