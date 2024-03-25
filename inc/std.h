#pragma once

#include <stdbool.h>

/*
 * ctype.h
 */
bool std_isdigit(char c);

/*
 * stdio.h
 */
void std_putc(char c);
void std_puts(const char* s);
void std_printf(const char* format, ...);

/*
 * stdlib.h
 */
char* std_itoa(int n, char* buffer, int base);

/*
 * string.h
 */
int std_strlen(const char* s);
char* std_strrev(char* s);
