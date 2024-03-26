#include "miros.h"
#include "std.h"
#include "stm32.h"

semaphore_t semaphore;

uint32_t thread1_stack[40];
thread_t thread1_thread;
void thread1_main(void) {
	for (int count = 0;; count++) {
		semaphore_wait(&semaphore);
		std_printf("Thread 1: %d\n", count);
		semaphore_signal(&semaphore);
		os_yield();
	}
}

uint32_t thread2_stack[40];
thread_t thread2_thread;
void thread2_main(void) {
	for (int count = 0;; count++) {
		semaphore_wait(&semaphore);
		std_printf("Thread 2: %d\n", count);
		semaphore_signal(&semaphore);
		os_yield();
	}
}

void main(void) {
	os_init();
	usart_init(USART1, 72e6 / 115200);

	semaphore_init(&semaphore, 1);
	thread_init(&thread1_thread, 1, &thread1_main, thread1_stack, sizeof(thread1_stack));
	thread_init(&thread2_thread, 2, &thread2_main, thread2_stack, sizeof(thread2_stack));

	os_run();
}
