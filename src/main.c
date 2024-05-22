#include "miros.h"
#include "stm32.h"

void aperiodic_main(void) {
	gpio_write(GPIOC, 13, true);
	os_burn(OS_SECONDS(1));
	gpio_write(GPIOC, 13, false);
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

	os_init(3);

	correr_thread = (thread_t) {
		.stack_begin = &correr_stack[sizeof(correr_stack)],
		.entry_point = &correr_main,
		.relative_deadline = OS_SECONDS(6),
		.period = OS_SECONDS(6),
	};
	os_add_thread(&correr_thread);

	agua_thread = (thread_t) {
		.stack_begin = &agua_stack[sizeof(agua_stack)],
		.entry_point = &agua_main,
		.relative_deadline = OS_SECONDS(8),
		.period = OS_SECONDS(8),
	};
	os_add_thread(&agua_thread);

	descanso_thread = (thread_t) {
		.stack_begin = &descanso_stack[sizeof(descanso_stack)],
		.entry_point = &descanso_main,
		.relative_deadline = OS_SECONDS(12),
		.period = OS_SECONDS(12),
	};
	os_add_thread(&descanso_thread);

	os_start();
}
