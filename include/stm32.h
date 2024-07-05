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
struct nvic {
	volatile uint32_t iser[1]; // Interrupt set enable register
	uint32_t reserved0[31];
	volatile uint32_t icer[1]; // Interrupt clear enable register
	uint32_t reserved1[31];
	volatile uint32_t ispr[1]; // Interrupt set pending register
	uint32_t reserved2[31];
	volatile uint32_t icpr[1]; // Interrupt clear pending register
	uint32_t reserved3[31];
	uint32_t reserved4[64];
	volatile uint32_t ip[1]; // Interrupt priority
};

#define NVIC ((struct nvic*) 0xE000E100)

typedef enum {
	IRQN_PENDSV = -2,
	IRQN_SYSTICK = -1,
	IRQN_EXTI9_5 = 23,
} IRQN;

#define NVIC_PRIO_BITS 4

void nvic_enable_irq(IRQN irqn);
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

#define RCC_APB1RSTR_I2C1RST (1 << 21)

#define RCC_APB2ENR_AFIOEN (1 << 0)
#define RCC_APB2ENR_IOPAEN (1 << 2)
#define RCC_APB2ENR_IOPBEN (1 << 3)
#define RCC_APB2ENR_IOPCEN (1 << 4)
#define RCC_APB2ENR_IOPEEN (1 << 6)
#define RCC_APB2ENR_USART1EN (1 << 14)

#define RCC_APB1ENR_TIM2EN (1 << 0)
#define RCC_APB1ENR_I2C1EN (1 << 21)

void rcc_init(void);
uint32_t rcc_get_clock(void);

// External interrupt event controller (EXTI)
struct exti {
	volatile uint32_t imr; // Interrupt mask register
	volatile uint32_t emr; // Event mask register
	volatile uint32_t rtsr; // Rising trigger selection register
	volatile uint32_t ftsr; // Falling trigger selection register
	volatile uint32_t swier; // Software interrupt event register
	volatile uint32_t pr; // Pending register
};

#define EXTI ((struct exti*) 0x40010400)

#define EXTI_TRIGGER_RISING (1 << 0)
#define EXTI_TRIGGER_FALLING (1 << 1)

void exti_enable(uint8_t line);
void exti_configure(uint8_t line, uint8_t trigger);
void exti_clear_pending(uint8_t line);

// General purpose input output (GPIO)
struct gpio {
	volatile uint32_t cr[2]; // Port configuration register
	volatile uint32_t idr; // Port input data register
	volatile uint32_t odr; // Port output data register
	volatile uint32_t bsrr; // Port bit set/reset register
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
void gpio_configure(struct gpio* gpio, uint8_t pin, uint8_t mode, uint8_t cnf);
void gpio_write(struct gpio* gpio, uint8_t pin, bool value);
bool gpio_read(struct gpio* gpio, uint8_t pin);

// I2C
struct i2c {
	volatile uint32_t cr1; // Control register 1
	volatile uint32_t cr2; // Control register 2
	volatile uint32_t oar1;
	volatile uint32_t oar2;
	volatile uint32_t dr;
	volatile uint32_t sr1;
	volatile uint32_t sr2;
	volatile uint32_t ccr;
	volatile uint32_t trise;
};

#define I2C1 ((struct i2c*) 0x40005400)

#define I2C_CR1_PE (1 << 0)
#define I2C_CR1_START (1 << 8)
#define I2C_CR1_STOP (1 << 9)
#define I2C_CR1_ACK (1 << 10)
#define I2C_CR1_POS (1 << 11)

#define I2C_CR2_FREQ(x) ((x) & 0b111111)

#define I2C_SR1_SB (1 << 0)
#define I2C_SR1_ADDR (1 << 1)
#define I2C_SR1_BTF (1 << 2)
#define I2C_SR1_RXNE (1 << 6)
#define I2C_SR1_TXE (1 << 7)
#define I2C_SR1_BERR (1 << 8)
#define I2C_SR1_ARLO (1 << 9)
#define I2C_SR1_AF (1 << 10)
#define I2C_SR1_OVR (1 << 11)
#define I2C_SR1_PECERR (1 << 12)
#define I2C_SR1_TIMEOUT (1 << 14)

void i2c_init(struct i2c* i2c);

void i2c_read(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size);
void i2c_write(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size);

// Timer
struct timer {
	volatile uint32_t cr1; // Control register 1
	volatile uint32_t cr2; // Control register 2
	volatile uint32_t smcr; // Slave mode control register
	volatile uint32_t dier; // DMA/interrupt enable register
	volatile uint32_t sr; // Status register
	volatile uint32_t egr; // Event generation register
	volatile uint32_t ccmr1; // Capture/compare mode register 1
	volatile uint32_t ccmr2; // Capture/compare mode register 2
	volatile uint32_t ccer; // Capture/compare enable register
	volatile uint32_t cnt; // Counter register
	volatile uint32_t psc; // Prescaler register
	volatile uint32_t arr; // Auto-reload register
	volatile uint32_t rcr; // Repetition counter register
	volatile uint32_t ccr1; // Capture/compare register 1
	volatile uint32_t ccr2; // Capture/compare register 2
	volatile uint32_t ccr3; // Capture/compare register 3
	volatile uint32_t ccr4; // Capture/compare register 4
	volatile uint32_t bdtr; // Break and dead-time register
	volatile uint32_t dcr; // DMA control register
	volatile uint32_t dmar; // DMA address for full transfer register
	volatile uint32_t or; // Option register
};

#define TIMER2 ((struct timer*) 0x40000000)

#define TIMER_CR1_CEN (1 << 0)

#define TIMER_CCMR1_OC2M_1 (2 << 12)
#define TIMER_CCMR1_OC2M_2 (4 << 12)

#define TIMER_CCER_CC2E (1 << 4)

void timer_init(struct timer* timer);

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

void usart_init(struct usart* usart, uint32_t brr);
void usart_write(struct usart* usart, char c);
char usart_read(struct usart* usart);
