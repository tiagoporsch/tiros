#include "cm3.h"
#include "miros.h"

#define OS_ONE_SECOND 100

void q_onAssert(const char* module, int line) {
	uart_printf(UART1, "ASSERT FAILED %s:%d\n", module, line);
	for (;;);
}

/*
 * Threads
 */
uint32_t blinky1_stack[40];
OSThread blinky1_thread;
void blinky1_main(void) {
	int count = 0;
	for (;;) {
		uart_printf(UART1, "Blinky1: %d\n", count++);
		os_delay(OS_ONE_SECOND * 3);
	}
}

uint32_t blinky2_stack[40];
OSThread blinky2_thread;
void blinky2_main(void) {
	int count = 0;
	for (;;) {
		uart_printf(UART1, "Blinky2: %d\n", count++);
		os_delay(OS_ONE_SECOND * 2);
	}
}

uint32_t blinky3_stack[40];
OSThread blinky3_thread;
void blinky3_main(void) {
	int count = 0;
	for (;;) {
		uart_printf(UART1, "Blinky3: %d\n", count++);
		os_delay(OS_ONE_SECOND * 1);
	}
}

/*
 * Main
 */
int main(void) {
	os_init();

	os_createThread(&blinky1_thread, 5, &blinky1_main, blinky1_stack, sizeof(blinky1_stack));
	os_createThread(&blinky2_thread, 2, &blinky2_main, blinky2_stack, sizeof(blinky2_stack));
	os_createThread(&blinky3_thread, 1, &blinky3_main, blinky3_stack, sizeof(blinky3_stack));

	rcc_init();
	uart_init(UART1, 115200);
	systick_init(RCC_SYS_CLOCK / OS_ONE_SECOND);
	nvic_setPriotity(-1, 0x00U);

	os_run();
}
