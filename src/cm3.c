#include "cm3.h"

#include <stdarg.h>

/*
 * NVIC
 */
void nvic_set_priority(int irq, uint32_t priority) {
	if (irq >= 0) {
	} else {
		SCB->shp[(((uint32_t) irq) & 0xFUL) - 4UL] = (uint8_t)((priority << (8U - NVIC_PRIO_BITS)) & (uint32_t) 0xFFUL);
	}
}

/*
 * SYSTICK
 */
void systick_init(unsigned long delay) {
	SYSTICK->reload = delay - 1;
	SYSTICK->value = 0UL;
	SYSTICK->csr = SYSTICK_SYSCLK | SYSTICK_ENABLE | SYSTICK_INTPEND;
}

/*
 * RCC
 */
void rcc_init(void) {
	// Configure the clock to 72 MHz
	RCC->cfg = RCC_PLL_HSE | RCC_PLL_9 | RCC_SYS_HSI | RCC_APB1_DIV2;
	RCC->ccr = RCC_HSI_ON | RCC_HSE_ON | RCC_HSE_TRIM | RCC_PLL_ENABLE;
	while (!(RCC->ccr & RCC_PLL_LOCK));
	*((volatile uint32_t*) 0x40022000) = 0x12;
	RCC->cfg = RCC_PLL_HSE | RCC_PLL_9 | RCC_SYS_PLL | RCC_APB1_DIV2;

	// Enable GPIOs
	RCC->ape2 |= RCC_GPIOA_ENABLE;
	RCC->ape2 |= RCC_GPIOC_ENABLE;

	// Enable UARTs
	RCC->ape2 |= RCC_UART1_ENABLE;
}

/*
 * GPIO
 */
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
	// 1 start bit, even parity
	uart->cr1 = 0x340C;
	uart->cr2 = 0;
	uart->cr3 = 0;
	uart->gtp = 0;
	uart->baud = (uart == UART1 ? 72000000 : 36000000) / baud;
}

void uart_putc(struct uart* uart, int c) {
	if (c == '\n')
		uart_putc(uart, '\r');
	while (!(uart->status & ST_TXE));
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
