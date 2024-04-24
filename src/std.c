#include "std.h"

#include <stdarg.h>

#include "stm32.h"

/*
 * ctype.h
 */
int isdigit(int c) {
	return c >= '0' && c <= '9';
}

/*
 * stdio.h
 */
int putchar(int c) {
	usart_write(USART1, c);
	return (unsigned char) c;
}

int puts(const char* s) {
	int written = 0;
	while (s[written])
		putchar(s[written++]);
	putchar('\n');
	return written + 1;
}

int printf(const char* restrict format, ...) {
	va_list args;
	va_start(args, format);

	static char buffer[33];
	int written = 0;
	while (*format != 0) {
		if (*format != '%') {
			putchar(*format++);
			written++;
			continue;
		}
		if (*++format == '%') {
			putchar('%');
			written++;
			continue;
		}
		char pad_char;
		int pad_count = 0;
		if (isdigit(*format)) {
			pad_char = *format == '0' ? '0' : ' ';
			do {
				pad_count *= 10;
				pad_count += (*format++ - '0');
			} while (isdigit(*format));
		}
		const char* s;
		switch (*format++) {
			case 's': s = va_arg(args, char*); break;
			case 'b': s = itoa(va_arg(args, int), buffer, 2); break;
			case 'o': s = itoa(va_arg(args, int), buffer, 8); break;
			case 'd': s = itoa(va_arg(args, int), buffer, 10); break;
			case 'x': s = itoa(va_arg(args, int), buffer, 16); break;
			default: s = "%?"; break;
		}
		for (int i = pad_count - strlen(s); i > 0; --i) {
			putchar(pad_char);
			written++;
		}
		while (*s) {
			putchar(*s++);
			written++;
		}
	}
	va_end(args);
	return written;
}

/*
 * stdlib.h
 */
char* itoa(int i, char* buffer, int base) {
	bool negative = (base == 10 && i < 0);
	if (negative)
		i = -i;
	int index = 0;
	do {
		int r = i % base;
		buffer[index++] = r > 9 ? ('a' + r - 10) : ('0' + r);
		i /= base;
	} while (i != 0);
	if (negative)
		buffer[index++] = '-';
	buffer[index++] = '\0';
	return strrev(buffer);
}

/*
 * string.h
 */
void* memset(void* dest, int c, size_t n) {
	unsigned char* d = (unsigned char*) dest;
	while (n-- > 0)
		*d++ = c;
	return dest;
}

size_t strlen(const char* s) {
	size_t len = 0;
	while (s[len] != 0)
		len++;
	return len;
}

char* strrev(char* s) {
	int len = strlen(s);
	for (int i = 0, j = len - 1; i <= j; i++, j--) {
		char c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
	return s;
}
