#pragma once

#include <stddef.h>

/*
 * ctype.h
 */
int isdigit(int c);

/*
 * stdio.h
 */
int putchar(int c);
int puts(const char* s);
int printf(const char* restrict format, ...);

/*
 * stdlib.h
 */
char* itoa(int n, char* buffer, int base);

/*
 * string.h
 */
void* memset(void* dest, int c, size_t n);
size_t strlen(const char* s);
char* strrev(char* s);
