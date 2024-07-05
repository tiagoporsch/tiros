#include "stm32.h"

/*
 * Cortex-M3
 */
// Nested vectored interrupt controller (NVIC)
void nvic_enable_irq(IRQN irqn) {
	if (irqn >= 0) {
		NVIC->iser[irqn >> 5] = 1 << (irqn & 0x1F);
	}
}

void nvic_set_priority(IRQN irqn, uint32_t priority) {
	if (irqn >= 0) {
	} else {
		SCB->shp[(((uint32_t) irqn) & 0xF) - 4] = (uint8_t)((priority << (8 - NVIC_PRIO_BITS)) & (uint32_t) 0xFF);
	}
}

// SysTick
void systick_init(uint32_t ticks) {
	SYSTICK->rvr = ticks - 1;
	SYSTICK->cvr = 0;
	SYSTICK->csr = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
}

/*
 * STM32F103
 */
// Reset and clock control (RCC)
void rcc_init(void) {
	// Configure the clock to 72 MHz
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL(7) | RCC_CFGR_SW(0b00) | RCC_CFGR_PPRE1(0b100);
	RCC->cr = RCC_CR_HSION | RCC_CR_HSITRIM(16) | RCC_CR_HSEON | RCC_CR_PLLON;
	while (!(RCC->cr & RCC_CR_PLLRDY));
	FLASH->acr = FLASH_ACR_LATENCY(0b010) | FLASH_ACR_PRFTBE;
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL(7) | RCC_CFGR_SW(0b10) | RCC_CFGR_PPRE1(0b100);
}

uint32_t rcc_get_clock(void) {
	return 72e6;
}

// External interrupt event controller (EXTI)
void exti_enable(uint8_t line) {
	EXTI->imr |= (1 << line);
}

void exti_configure(uint8_t line, uint8_t trigger) {
	if (trigger & EXTI_TRIGGER_RISING)
		EXTI->rtsr |= (1 << line);
	else
		EXTI->rtsr &= ~(1 << line);
	if (trigger & EXTI_TRIGGER_FALLING)
		EXTI->ftsr |= (1 << line);
	else
		EXTI->ftsr &= ~(1 << line);
}

void exti_clear_pending(uint8_t line) {
	EXTI->pr |= (1 << line);
}

// General purpose input output (GPIO)
void gpio_init(struct gpio* gpio) {
	switch ((uint32_t) gpio) {
		case (uint32_t) GPIOA: RCC->apb2enr |= RCC_APB2ENR_IOPAEN; break;
		case (uint32_t) GPIOB: RCC->apb2enr |= RCC_APB2ENR_IOPBEN; break;
		case (uint32_t) GPIOC: RCC->apb2enr |= RCC_APB2ENR_IOPCEN; break;
	}
}

void gpio_configure(struct gpio* gpio, uint8_t pin, uint8_t mode, uint8_t cnf) {
	uint8_t reg = pin / 8;
	uint8_t base = (pin % 8) * 4;
	gpio->cr[reg] = (gpio->cr[reg] & ~(0b1111 << base)) | (mode << base) | (cnf << base << 2);
}

void gpio_write(struct gpio* gpio, uint8_t pin, bool value) {
	if (value)
		gpio->brr |= 1 << pin;
	else
		gpio->bsrr |= 1 << pin;
}

bool gpio_read(struct gpio* gpio, uint8_t pin) {
	return (gpio->idr >> pin) != 0;
}

// I2C
void i2c_init(struct i2c* i2c) {
	switch ((uint32_t) i2c) {
		case (uint32_t) I2C1:
			RCC->apb1enr |= RCC_APB1ENR_I2C1EN;
			RCC->apb2enr |= RCC_APB2ENR_IOPBEN;
			gpio_configure(GPIOB, 6, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_OPEN_DRAIN);
			gpio_configure(GPIOB, 7, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_OPEN_DRAIN);
			break;
	}
	i2c->cr2 = I2C_CR2_FREQ(36);
	i2c->ccr = 180;
	i2c->trise = 37;
	i2c->cr1 |= I2C_CR1_PE;
}

void i2c_read(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size) {
	i2c->cr1 &= ~I2C_CR1_POS;
	i2c->cr1 |= I2C_CR1_START | I2C_CR1_ACK;
	while (!(i2c->sr1 & I2C_SR1_SB));
	i2c->dr = (slave_address << 1) | 1;
	while (!(i2c->sr1 & I2C_SR1_ADDR));
	i2c->sr1;
	i2c->sr2;
	uint8_t index;
	for (index = 0; index < size; index++) {
		if (index + 1 == size) {
			i2c->cr1 &= ~I2C_CR1_ACK;
			i2c->cr1 |= I2C_CR1_STOP;
		}
		while (!(i2c->sr1 & I2C_SR1_RXNE));
		data[index] = i2c->dr;
	}
}

void i2c_write(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size) {
	volatile uint16_t reg;
	i2c->cr1 &= ~I2C_CR1_POS;
	i2c->cr1 |= I2C_CR1_START;
	while (!(i2c->sr1 & I2C_SR1_SB));
	i2c->dr = (slave_address << 1) | 0;
	while (!(i2c->sr1 & I2C_SR1_ADDR));
	reg = i2c->sr1;
	reg = i2c->sr2;
	while (!(i2c->sr1 & I2C_SR1_TXE));
	for (uint8_t index = 0; index < size; index++) {
		i2c->dr = data[index];
		while (!(i2c->sr1 & I2C_SR1_TXE));
		while (!(i2c->sr1 & I2C_SR1_BTF));
		reg = i2c->sr1;
		reg = i2c->sr2;
	}
	i2c->cr1 |= I2C_CR1_STOP;
	reg = i2c->sr1;
	reg = i2c->sr2;
	(void) reg;
}

// Timer
void timer_init(struct timer* timer) {
	switch ((uint32_t) timer) {
		case (uint32_t) TIMER2: RCC->apb1enr |= RCC_APB1ENR_TIM2EN; break;
	}
}

// Universal synchronous asynchronous receiver transmitter (USART)
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
