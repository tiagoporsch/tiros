#include "cm3.h"
#include "miros.h"

#define OS_ONE_SECOND 100

void Q_onAssert(const char* module, int loc) {
	uart_printf(UART1, "ASSERT FAILED %s:%d\n", module, loc);
	for (;;);
}

void os_onStartup(void) {
	rcc_init();
	gpio_mode(GPIOC, 13, GPIO_MODE_OUTPUT_2M | GPIO_MODE_OUTPUT_OPEN_DRAIN); // LED
	gpio_mode(GPIOA, 9, GPIO_MODE_OUTPUT_50M | GPIO_MODE_ALT_PUSH_PULL); // UART
	uart_init(UART1, 115200);
	uart_puts(UART1, "\nHello from MirOS!\n");
	systick_init(RCC_SYS_CLOCK / OS_ONE_SECOND);
	nvic_set_priority(-1, 0x00U);
}

void os_onIdle(void) {
}

void systick_handler(void) {
	os_tick();
	asm volatile ("cpsid i" : : : "memory");
	os_sched();
	asm volatile ("cpsie i" : : : "memory");
}

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
		os_delay(OS_ONE_SECOND);
	}
}

uint32_t blinky3_stack[40];
OSThread blinky3_thread;
void blinky3_main(void) {
	int count = 0;
	for (;;) {
		uart_printf(UART1, "Blinky3: %d\n", count++);
		os_delay(OS_ONE_SECOND * 2);
	}
}

int main(void) {
	os_init();
	os_createThread(&blinky1_thread, 5, &blinky1_main, blinky1_stack, sizeof(blinky1_stack));
	os_createThread(&blinky2_thread, 2, &blinky2_main, blinky2_stack, sizeof(blinky2_stack));
	os_createThread(&blinky3_thread, 1, &blinky3_main, blinky3_stack, sizeof(blinky3_stack));
	os_run();
}
