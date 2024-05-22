#include "miros.h"
#include "stm32.h"

void aperiodic_main(void) {
	gpio_write(GPIOC, 13, true);
	os_burn(OS_MILLIS(500));
	gpio_write(GPIOC, 13, false);
	os_burn(OS_MILLIS(500));
}

void exti9_5_handler(void) {
	exti_clear_pending(8);

	// 100 ms debouncer
	static uint32_t last_millis = 0;
	if (os_current_millis() - last_millis < 100)
		return;
	last_millis = os_current_millis();

	os_enqueue_aperiodic_task(&aperiodic_main, OS_SECONDS(1));
}

thread_t task1_thread;
uint8_t task1_stack[256] __attribute__ ((aligned(8)));
void task1_main(void) {
	os_burn(OS_SECONDS(3));
}

thread_t task2_thread;
uint8_t task2_stack[256] __attribute__ ((aligned(8)));
void task2_main(void) {
	os_burn(OS_SECONDS(2));
}

int main(void) {
	// Configure LED (C13)
	gpio_init(GPIOC);
	gpio_configure(GPIOC, 13, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_PUSH_PULL);
	gpio_write(GPIOC, 13, false);

	// Configure button (A8) interrupt
	gpio_init(GPIOA);
	gpio_configure(GPIOA, 8, GPIO_CR_MODE_INPUT, GPIO_CR_CNF_INPUT_FLOATING);
	exti_configure(8, EXTI_TRIGGER_RISING);
	exti_enable(8);
	nvic_enable_irq(IRQN_EXTI9_5);

	os_init(4);

	task1_thread = (thread_t) {
		.stack_begin = &task1_stack[sizeof(task1_stack)],
		.entry_point = &task1_main,
		.relative_deadline = OS_SECONDS(6),
		.period = OS_SECONDS(6),
	};
	os_add_thread(&task1_thread);

	task2_thread = (thread_t) {
		.stack_begin = &task2_stack[sizeof(task2_stack)],
		.entry_point = &task2_main,
		.relative_deadline = OS_SECONDS(8),
		.period = OS_SECONDS(8),
	};
	os_add_thread(&task2_thread);

	os_start();
}
