#include "miros.h"

thread_t correr_thread;
uint8_t correr_stack[256] __attribute__ ((aligned(8)));
void correr_main(void) {
	os_burn(OS_SECONDS(2));
}

thread_t agua_thread;
uint8_t agua_stack[256] __attribute__ ((aligned(8)));
void agua_main(void) {
	os_burn(OS_SECONDS(2));
}

thread_t descanso_thread;
uint8_t descanso_stack[256] __attribute__ ((aligned(8)));
void descanso_main(void) {
	os_burn(OS_SECONDS(4));
}

int main(void) {
	os_init();

	correr_thread = (thread_t) {
		.stack_begin = &correr_stack[sizeof(correr_stack)],
		.entry_point = &correr_main,
		.relative_deadline = OS_SECONDS(5),
		.period = OS_SECONDS(6),
		.debug_pin = 9
	};
	os_thread_add(&correr_thread);

	agua_thread = (thread_t) {
		.stack_begin = &agua_stack[sizeof(agua_stack)],
		.entry_point = &agua_main,
		.relative_deadline = OS_SECONDS(4),
		.period = OS_SECONDS(8),
		.debug_pin = 10
	};
	os_thread_add(&agua_thread);

	descanso_thread = (thread_t) {
		.stack_begin = &descanso_stack[sizeof(descanso_stack)],
		.entry_point = &descanso_main,
		.relative_deadline = OS_SECONDS(8),
		.period = OS_SECONDS(12),
		.debug_pin = 11
	};
	os_thread_add(&descanso_thread);

	os_start();
}
