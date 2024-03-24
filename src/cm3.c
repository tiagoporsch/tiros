#include "cm3.h"

/*
 * Nested vectored interrupt controller (NVIC)
 */
void nvic_set_priority(IRQN irqn, uint32_t priority) {
	if (irqn >= 0) {
	} else {
		SCB->shp[(((uint32_t) irqn) & 0xF) - 4] = (uint8_t)((priority << (8 - NVIC_PRIO_BITS)) & (uint32_t) 0xFF);
	}
}

/*
 * SysTick
 */
void systick_init(uint32_t ticks) {
	SYSTICK->rvr = ticks - 1;
	SYSTICK->cvr = 0;
	SYSTICK->csr = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
}

/*
 * Reset and clock control (RCC)
 */
void rcc_init(void) {
	// Configure the clock to 72 MHz
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL(7) | RCC_CFGR_SW(0b00) | RCC_CFGR_PPRE1(0b100);
	RCC->cr = RCC_CR_HSION | RCC_CR_HSITRIM(16) | RCC_CR_HSEON | RCC_CR_PLLON;
	while (!(RCC->cr & RCC_CR_PLLRDY));
	FLASH->acr = FLASH_ACR_LATENCY(0b010) | FLASH_ACR_PRFTBE;
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL(7) | RCC_CFGR_SW(0b10) | RCC_CFGR_PPRE1(0b100);
}

/*
 * General purpose input output (GPIO)
 */
void gpio_init(struct gpio* gpio) {
	switch ((uint32_t) gpio) {
		case (uint32_t) GPIOA: RCC->apb2enr |= RCC_APB2ENR_IOPAEN; break;
		case (uint32_t) GPIOB: RCC->apb2enr |= RCC_APB2ENR_IOPBEN; break;
		case (uint32_t) GPIOC: RCC->apb2enr |= RCC_APB2ENR_IOPCEN; break;
	}
}

void gpio_configure(struct gpio* gpio, int pin, int mode, int configuration) {
	int reg = pin / 8;
	int base = (pin % 8) * 4;
	gpio->cr[reg] = (gpio->cr[reg] & ~(0xF << base)) | (mode << base) | (configuration << base << 2);
}

/*
 * Universal synchronous asynchronous receiver transmitter (USART)
 */
void usart_init(struct usart* usart, uint32_t brr) {
	switch ((uint32_t) usart) {
		case (uint32_t) USART1:
			RCC->apb2enr |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
			gpio_configure(GPIOA, 9, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_PUSH_PULL);
			gpio_configure(GPIOA, 10, GPIO_CR_MODE_INPUT, GPIO_CR_CNF_INPUT_FLOATING);
			break;
	}
	usart->cr1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_PCE | USART_CR1_M | USART_CR1_UE;
	usart->cr2 = 0;
	usart->cr3 = 0;
	usart->gtpr = 0;
	usart->brr = brr;
}

void usart_write(struct usart* usart, char c) {
	while (!(usart->sr & USART_SR_TXE));
	usart->dr = c;
}

char usart_read(struct usart* usart) {
	while (!(usart->sr & USART_SR_RXNE));
	return usart->dr;
}
