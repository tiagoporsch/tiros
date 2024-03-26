#include "miros.h"
#include "std.h"
#include "stm32.h"

#define QUEUE_SIZE 4
struct {
	int head;
	int tail;
	int data[QUEUE_SIZE];
} queue;

semaphore_t semaphore_occupied;
semaphore_t semaphore_available;

uint32_t producer_stack[40];
thread_t producer_thread;
void producer_main(void) {
	for (int value = 0;; value++) {
		semaphore_wait(&semaphore_available);
		queue.data[queue.tail] = value;
		if (++queue.tail == QUEUE_SIZE)
			queue.tail = 0;
		semaphore_signal(&semaphore_occupied);
		if (!(value % 3))
			os_delay(OS_ONE_SECOND);
	}
}

uint32_t consumer_stack[40];
thread_t consumer_thread;
void consumer_main(void) {
	for (;;) {
		semaphore_wait(&semaphore_occupied);
		int value = queue.data[queue.head];
		std_printf("consumed %d\n", value);
		if (++queue.head== QUEUE_SIZE)
			queue.head = 0;
		semaphore_signal(&semaphore_available);
		if (!(value % 5))
			os_delay(OS_ONE_SECOND);
	}
}

void main(void) {
	os_init();
	usart_init(USART1, 72e6 / 115200);

	semaphore_init(&semaphore_occupied, QUEUE_SIZE, 0);
	semaphore_init(&semaphore_available, QUEUE_SIZE, QUEUE_SIZE);
	thread_init(&producer_thread, 1, &producer_main, producer_stack, sizeof(producer_stack));
	thread_init(&consumer_thread, 2, &consumer_main, consumer_stack, sizeof(consumer_stack));

	os_run();
}
