#include "cm3.h"
#include "miros.h"

Semaphore semaphore;

uint32_t producer_stack[40];
Thread producer_thread;
void producer_main(void) {
	int count = 0;
	for (;;) {
		uart_printf(UART1, "Producing %d\n", count++);
		semaphore_signal(&semaphore);
		os_delay(2 * OS_ONE_SECOND);
	}
}

uint32_t consumer_stack[40];
Thread consumer_thread;
void consumer_main(void) {
	int count = 0;
	for (;;) {
		semaphore_wait(&semaphore);
		uart_printf(UART1, "Consumed %d\n", count++);
	}
}

int main(void) {
	os_init();

	semaphore_init(&semaphore, 0);
	thread_init(&producer_thread, 5, &producer_main, producer_stack, sizeof(producer_stack));
	thread_init(&consumer_thread, 2, &consumer_main, consumer_stack, sizeof(consumer_stack));

	os_run();
}
