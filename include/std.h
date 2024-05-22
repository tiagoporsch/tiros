#pragma once

#include <stddef.h>

/*
 * ctype.h
 */
int std_isdigit(int c);

/*
 * stdio.h
 */
int std_putc(int c);
int std_puts(const char* s);
int std_printf(const char* restrict format, ...);

/*
 * stdlib.h
 */
char* std_itoa(int n, char* buffer, int base);

/*
 * string.h
 */
size_t std_strlen(const char* s);
char* std_strrev(char* s);
