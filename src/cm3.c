#include "cm3.h"

#include <stdarg.h>

/*
 * NVIC
 */
void nvic_setPriotity(int irq, uint32_t priority) {
	if (irq >= 0) {
	} else {
		SCB->shp[(((uint32_t) irq) & 0xFUL) - 4UL] = (uint8_t)((priority << (8U - NVIC_PRIO_BITS)) & (uint32_t) 0xFFUL);
	}
}

/*
 * SYSTICK
 */
void systick_init(uint32_t ticks) {
	SYSTICK->load = ticks - 1;
	SYSTICK->val = 0UL;
	SYSTICK->csr = SYSTICK_ENABLE | SYSTICK_TICKINT | SYSTICK_CLKSOURCE;
}

/*
 * RCC
 */
void rcc_init(void) {
	// Configure the clock to 72 MHz
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9 | RCC_CFGR_SW_HSI | RCC_CFGR_PPRE1_2;
	RCC->cr = RCC_CR_HSION | RCC_CR_HSITRIM | RCC_CR_HSEON | RCC_CR_PLLON;
	while (!(RCC->cr & RCC_CR_PLLRDY));
	*((volatile uint32_t*) 0x40022000) = 0x12;
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9 | RCC_CFGR_SW_PLL | RCC_CFGR_PPRE1_2;
}

/*
 * GPIO
 */
void gpio_init(struct gpio* gpio) {
	switch ((uint32_t) gpio) {
		case (uint32_t) GPIOA: RCC->ape2 |= RCC_GPIOA_ENABLE; break;
		case (uint32_t) GPIOB: RCC->ape2 |= RCC_GPIOB_ENABLE; break;
		case (uint32_t) GPIOC: RCC->ape2 |= RCC_GPIOC_ENABLE; break;
	}
}

void gpio_mode(struct gpio* gpio, int pin, int mode) {
	int reg = pin / 8;
	int shift = (pin % 8) * 4;
	int conf = gpio->cr[reg] & ~(0xf << shift);
	gpio->cr[reg] = conf | (mode << shift);
}

void gpio_on(struct gpio* gpio, int pin) {
	gpio->bsrrh |= 1 << pin;
}

void gpio_off(struct gpio* gpio, int pin) {
	gpio->bsrrl |= 1 << pin;
}

/*
 * UART
 */
void uart_init(struct uart* uart, int baud) {
	switch ((uint32_t) uart) {
		case (uint32_t) UART1:
			RCC->ape2 |= RCC_UART1_ENABLE;
			gpio_init(GPIOA);
			gpio_mode(GPIOA, 9, GPIO_MODE_OUTPUT_50M | GPIO_MODE_ALT_PUSH_PULL);
			break;
	}
	uart->cr1 = 0x340C; // 1 start bit, even parity
	uart->cr2 = 0;
	uart->cr3 = 0;
	uart->gtp = 0;
	uart->baud = RCC_SYS_CLOCK / baud;
}

void uart_putc(struct uart* uart, int c) {
	if (c == '\n')
		uart_putc(uart, '\r');
	while (!(uart->status & UART_TXE));
	uart->data = c;
}

void uart_puts(struct uart* uart, const char* s) {
	while (*s)
		uart_putc(uart, *s++);
}

static const char* itoa(int i, int base, int pad, char padchar) {
	static const char* repr = "0123456789ABCDEF";
	static char buf[33] = { 0 };
	char* ptr = &buf[32];
	*ptr = 0;
	do {
		*--ptr = repr[i % base];
		if (pad) pad--;
		i /= base;
	} while (i != 0);
	while (pad--) *--ptr = padchar;
	return ptr;
}

void uart_printf(struct uart* uart, const char* format, ...) {
	va_list args;
	va_start(args, format);
	int pad = 0;
	char padchar = ' ';
	while (*format) {
		if (*format == '%' || pad != 0) {
			if (*format == '%')
				format++;
			if (*format == '%') {
				uart_putc(uart, (char) va_arg(args, int));
				format++;
			} else if (*format == 's') {
				uart_puts(uart, va_arg(args, char*));
				format++;
			} else if (*format == 'd') {
				int number = va_arg(args, int);
				if (number < 0) {
					number = -number;
					uart_putc(uart, '-');
				}
				uart_puts(uart, itoa(number, 10, pad, padchar));
				pad = 0;
				format++;
			} else if (*format == 'x') {
				uart_puts(uart, itoa(va_arg(args, int), 16, pad, padchar));
				pad = 0;
				format++;
			} else if (*format >= '0' && *format <= '9') {
				if (*format == '0') {
					padchar = '0';
					format++;
				} else {
					padchar = ' ';
				}
				while (*format >= '0' && *format <= '9') {
					pad *= 10;
					pad += (*format++ - '0');
				}
			}
		} else {
			uart_putc(uart, *format++);
		}
	}
	va_end(args);
}
