#include "miros.h"
#include "std.h"
#include "stm32.h"

uint32_t task1_stack[128];
thread_t task1_thread;
void task1_main(void) {
	std_printf("task1\n");
}

uint32_t task2_stack[128];
thread_t task2_thread;
void task2_main(void) {
	std_printf("task2\n");
}

void main(void) {
	rcc_init();
	usart_init(USART1, 72e6 / 115200);
	os_init();

	thread_init(&task1_thread, task1_stack, sizeof(task1_stack), &task1_main, OS_SECONDS(1), OS_SECONDS(1));
	thread_init(&task2_thread, task2_stack, sizeof(task2_stack), &task2_main, OS_SECONDS(1), OS_SECONDS(2));

	os_start();
}
