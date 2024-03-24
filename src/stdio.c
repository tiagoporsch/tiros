#include "stdio.h"

#include <stdarg.h>

#include "cm3.h"

void putc(char c) {
	if (c == '\n')
		putc('\r');
	usart_write(USART1, c);
}

void puts(const char* s) {
	while (*s)
		putc(*s++);
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

void printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	int pad = 0;
	char padchar = ' ';
	while (*format) {
		if (*format == '%' || pad != 0) {
			if (*format == '%')
				format++;
			if (*format == '%') {
				putc((char) va_arg(args, int));
				format++;
			} else if (*format == 's') {
				puts(va_arg(args, char*));
				format++;
			} else if (*format == 'd') {
				int number = va_arg(args, int);
				if (number < 0) {
					number = -number;
					putc('-');
				}
				puts(itoa(number, 10, pad, padchar));
				pad = 0;
				format++;
			} else if (*format == 'x') {
				puts(itoa(va_arg(args, int), 16, pad, padchar));
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
			putc(*format++);
		}
	}
	va_end(args);
}
