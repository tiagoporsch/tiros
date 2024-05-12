#include "runtime.h"

void* memset(void* s, int c, size_t n) {
	unsigned char* d = (unsigned char*) s;
	while (n-- > 0)
		*d++ = c;
	return s;
}
