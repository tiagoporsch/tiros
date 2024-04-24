#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * ARMv7
 */
__attribute__((always_inline)) static inline void __disable_irq(void) {
	asm volatile ("cpsid i" : : : "memory");
}

__attribute__((always_inline)) static inline void __enable_irq(void) {
	asm volatile ("cpsie i" : : : "memory");
}

/*
 * Cortex-M3
 */
// Nested vectored interrupt controller (NVIC)
typedef enum {
	IRQN_PENDSV = -2,
	IRQN_SYSTICK = -1,
} IRQN;

#define NVIC_PRIO_BITS 4

void nvic_set_priority(IRQN irqn, uint32_t priority);

// System control block (SCB)
struct scb {
	volatile uint32_t cpuid; // CPUID Base Register
	volatile uint32_t icsr; // Interrupt Control and State Register
	volatile uint32_t vtor; // Vector Table Offset Register
	volatile uint32_t aircr; // Application Interrupt and Reset Control Register
	volatile uint32_t scr; // System Control Register
	volatile uint32_t ccr; // Configuration Control Register
	volatile uint8_t shp[12]; // System Handlers Priority Registers
	volatile uint32_t shcsr; // System Handler Control and State Register
	volatile uint32_t cfsr; // Configurable Fault Status Register
};

#define SCB ((struct scb*) 0xE000ED00)

#define SCB_ICSR_PENDSVSET (1 << 28)

// SysTick
struct systick {
	volatile uint32_t csr; // Control and Status Register
	volatile uint32_t rvr; // Reload Value Register
	volatile uint32_t cvr; // Current Value Register
	volatile uint32_t calib; // Calibration Register
};

#define SYSTICK ((struct systick*) 0xE000E010)

#define	SYSTICK_CSR_ENABLE (1 << 0)
#define	SYSTICK_CSR_TICKINT (1 << 1)
#define	SYSTICK_CSR_CLKSOURCE (1 << 2)

extern void systick_handler(void);
void systick_init(uint32_t ticks);

/*
 * STM32F103
 */
// Flash interface registers (FLASH)
struct flash {
	volatile uint32_t acr; // Access control register
	volatile uint32_t keyr; // Key register
	volatile uint32_t optkeyr; // Option key register
	volatile uint32_t sr; // Status register
	volatile uint32_t cr; // Control register
	volatile uint32_t ar; // Address register
	uint32_t reserved0;
	volatile uint32_t obr; // Option byte register
	volatile uint32_t wrpr; // Write protection register
};

#define FLASH ((struct flash*) 0x40022000)

#define FLASH_ACR_LATENCY(x) ((x) << 0)
#define FLASH_ACR_PRFTBE (1 << 4)

// Reset and Clock Control (RCC)
struct rcc {
	volatile uint32_t cr; // Clock control register
	volatile uint32_t cfgr; // Clock configuration register
	volatile uint32_t cir; // Clock interrupt register
	volatile uint32_t apb2rstr; // APB2 peripheral reset register
	volatile uint32_t apb1rstr; // APB1 peripheral reset register
	volatile uint32_t ahbenr; // AHB peripheral clock enable register
	volatile uint32_t apb2enr; // APB2 peripheral clock enable register
	volatile uint32_t apb1enr; // APB1 peripheral clock enable register
	volatile uint32_t bdcr; // Backup domain control register
	volatile uint32_t csr; // Control/status register
};

#define RCC ((struct rcc*) 0x40021000)

#define RCC_CR_HSION (1 << 0)
#define RCC_CR_HSITRIM(x) ((x) << 3)
#define RCC_CR_HSEON (1 << 16)
#define RCC_CR_PLLON (1 << 24)
#define RCC_CR_PLLRDY (1 << 25)

#define RCC_CFGR_SW(x) ((x) << 0)
#define RCC_CFGR_PPRE1(x) ((x) << 8)
#define RCC_CFGR_PLLSRC (1 << 16)
#define RCC_CFGR_PLLMULL(x) ((x) << 18)

#define RCC_APB2ENR_IOPAEN (1 << 2)
#define RCC_APB2ENR_IOPBEN (1 << 3)
#define RCC_APB2ENR_IOPCEN (1 << 4)
#define RCC_APB2ENR_USART1EN (1 << 14)

void rcc_init(void);

// General purpose input output (GPIO)
struct gpio {
	volatile uint32_t cr[2]; // Port configuration register
	volatile uint32_t idr; // Port input data register
	volatile uint32_t odr; // Port output data register
	volatile int bsrrl; // Port bit set/reset register
	volatile int bsrrh; // Port bit set/reset register
	volatile uint32_t brr; // Port bit reset register
	volatile uint32_t lckr; // Port configuration lock register
};

#define GPIOA ((struct gpio*) 0x40010800)
#define GPIOB ((struct gpio*) 0x40010C00)
#define GPIOC ((struct gpio*) 0x40011000)

#define GPIO_CR_MODE_INPUT 0b00
#define GPIO_CR_MODE_OUTPUT_10M 0b01
#define GPIO_CR_MODE_OUTPUT_2M 0b10
#define GPIO_CR_MODE_OUTPUT_50M 0b11

#define GPIO_CR_CNF_INPUT_ANALOG 0b00
#define GPIO_CR_CNF_INPUT_FLOATING 0b01
#define GPIO_CR_CNF_INPUT_PUPD 0b10
#define GPIO_CR_CNF_OUTPUT_PUSH_PULL 0b00
#define GPIO_CR_CNF_OUTPUT_OPEN_DRAIN 0b01
#define GPIO_CR_CNF_OUTPUT_ALT_PUSH_PULL 0b10
#define GPIO_CR_CNF_OUTPUT_ALT_OPEN_DRAIN 0b11

void gpio_init(struct gpio* gpio);
void gpio_configure(struct gpio* gpio, int pin, int mode, int cnf);
void gpio_set(struct gpio* gpio, int pin, bool on);

// Universal synchronous asynchronous receiver transmitter (USART)
struct usart {
	volatile uint32_t sr; // Status register
	volatile uint32_t dr; // Data register
	volatile uint32_t brr; // Baud rate register
	volatile uint32_t cr1; // Control register 1
	volatile uint32_t cr2; // Control register 2
	volatile uint32_t cr3; // Control register 3
	volatile uint32_t gtpr; // Guard time and prescaler
};

#define USART1 ((struct usart*) 0x40013800)
#define USART2 ((struct usart*) 0x40004400)
#define USART3 ((struct usart*) 0x40004800)

#define USART_SR_RXNE (1 << 5) // Read data register not empty
#define USART_SR_TXE (1 << 7) // Transmit data register empty

#define USART_CR1_RE (1 << 2) // Receiver enable
#define USART_CR1_TE (1 << 3) // Transmitter enable
#define USART_CR1_PCE (1 << 10) // Parity control enable
#define USART_CR1_M (1 << 12) // Word length
#define USART_CR1_UE (1 << 13) // USART enable

void usart_init(struct usart* usart, uint32_t baud);
void usart_write(struct usart* usart, char c);
char usart_read(struct usart* usart);
