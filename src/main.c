#include "miros.h"
#include "stm32.h"
#include "VL53L0X.h"

// Sensor
static struct VL53L0X myTOFsensor = {.io_2v8 = true, .address = 0b0101001, .io_timeout = 500, .did_timeout = false};

// Controller
static int CONTROLLER_PERIOD = OS_MILLIS(50);

// Inter-process communication
static int measured_value = 0; // mm
static int reference_value = 250; // mm
static int duty_cycle = 91; // [31~91] %

static semaphore_t measure_occupied_semaphore, measure_available_semaphore;
static semaphore_t actuator_occupied_semaphore, actuator_available_semaphore;

// PWM
static const uint32_t PMW_PRESCALE = 1e6; // Hz
static const uint32_t PWM_FREQUENCY = 25e3; // Hz

thread_t sensor_thread;
uint8_t sensor_stack[256] __attribute__ ((aligned(8)));
void sensor_main(void) {
	// Wait for the previously measured value to be consumed
	semaphore_wait(&measure_available_semaphore);
	// measured_value = VL53L0X_readRangeContinuousMillimeters(&myTOFsensor) - 400;
	// Signal that the newly measured value is ready
	semaphore_signal(&measure_occupied_semaphore);
}

thread_t controller_thread;
uint8_t controller_stack[256] __attribute__ ((aligned(8)));
void controller_main(void) {
	// Use these values to work around floating-point math
	static int kp = -10; // -0.00010
	static int ki = -1;  // -0.00001
	static int kd =  1;  //  0.00001

	static int error_sum = 0;
	static int previous_measured_value = 0;

	// Wait for the measured value to be ready
	semaphore_wait(&measure_occupied_semaphore);
	// Wait for the previously calculated value to be consumed
	semaphore_wait(&actuator_available_semaphore);

	int error = reference_value - measured_value;
	int output = kp * error;
	output += ki * error_sum * CONTROLLER_PERIOD;
	output -= kd * (measured_value - previous_measured_value) / CONTROLLER_PERIOD;

	previous_measured_value = measured_value;
	error_sum += error;
	if (error_sum < -1000)
		error_sum = -1000;
	if (error_sum > 1000)
		error_sum = 1000;

	if (output < -30000)
		output = -30000;
	else if (output > 30000)
		output = 30000;
	duty_cycle = (output + 61000) / 1000; // Compensate the floating point workaround

	// Signal that the newly calculated value is ready
	semaphore_signal(&actuator_occupied_semaphore);
	// Signal that the measured value was consumed
	semaphore_signal(&measure_available_semaphore);
}

thread_t actuator_thread;
uint8_t actuator_stack[256] __attribute__ ((aligned(8)));
void actuator_main(void) {
	// Wait for the calculated value to be ready
	semaphore_wait(&actuator_occupied_semaphore);
	// TIMER2->ccr2 = TIMER2->arr * duty_cycle / 100;
	// Signal that the calculated value was consumed
	semaphore_signal(&actuator_available_semaphore);
}

void change_main(void) {
	gpio_write(GPIOC, 13, true);
	switch (reference_value) {
		default:
			reference_value = 250;
			TIMER2->ccr2 = TIMER2->arr * 91 / 100;
			break;
		case 250:
			reference_value = 500;
			TIMER2->ccr2 = TIMER2->arr * 61 / 100;
			break;
		case 500:
			reference_value = 750;
			TIMER2->ccr2 = TIMER2->arr * 31 / 100;
			break;
	}
	for (int i = 0; i < 500000; i++);
	gpio_write(GPIOC, 13, false);
}

int main(void) {
	// Configure clock
	rcc_init();

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

	// Configure TIM2 for PWM output
	timer_init(TIMER2);
	gpio_init(GPIOA);
	gpio_configure(GPIOA, 1, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_PUSH_PULL);
	TIMER2->psc = rcc_get_clock() / PMW_PRESCALE - 1;
	TIMER2->arr = PMW_PRESCALE / PWM_FREQUENCY;
	TIMER2->ccr2 = TIMER2->arr * 91 / 100;
	TIMER2->ccmr1 |= TIMER_CCMR1_OC2M_1 | TIMER_CCMR1_OC2M_2; // PWM mode 1
	TIMER2->ccer |= TIMER_CCER_CC2E; // enable output compare on OC2 pin
	TIMER2->cr1 |= TIMER_CR1_CEN; // enable timer

	// Configure VL53L0X
	// usart_init(USART1, rcc_get_clock() / 115200);
	// VL53L0X_init(&myTOFsensor);
	// VL53L0X_setMeasurementTimingBudget(&myTOFsensor, 20e3); // 20 ms
	// VL53L0X_startContinuous(&myTOFsensor, 0);

	// Initialize operating system
	os_init(5);
	// 5 for maximum utilization because
	// 10/50 + 25/50 + 5/50 = 0.8
	// 1/(1-0.8) = 5

	// Semaphores
	semaphore_init(&measure_available_semaphore, 1, 1);
	semaphore_init(&measure_occupied_semaphore, 1, 0);
	semaphore_init(&actuator_available_semaphore, 1, 1);
	semaphore_init(&actuator_occupied_semaphore, 1, 0);

	sensor_thread = (thread_t) {
		.stack_begin = &sensor_stack[sizeof(sensor_stack)],
		.entry_point = &sensor_main,
		.relative_deadline = OS_MILLIS(10),
		.period = OS_MILLIS(50),
	};
	os_add_thread(&sensor_thread);

	controller_thread = (thread_t) {
		.stack_begin = &controller_stack[sizeof(controller_stack)],
		.entry_point = &controller_main,
		.relative_deadline = OS_MILLIS(25),
		.period = OS_MILLIS(50),
	};
	os_add_thread(&controller_thread);

	actuator_thread = (thread_t) {
		.stack_begin = &actuator_stack[sizeof(actuator_stack)],
		.entry_point = &actuator_main,
		.relative_deadline = OS_MILLIS(5),
		.period = OS_MILLIS(50),
	};
	os_add_thread(&actuator_thread);

	os_start();
}

void exti9_5_handler(void) {
	exti_clear_pending(8);

	// 200 ms debouncer
	static uint32_t last_millis = 0;
	if (os_current_millis() - last_millis < 200)
		return;
	last_millis = os_current_millis();

	// When the pull-up button wired to pin A8 gets pulled high
	// enqueue an aperiodic task with computation time of 1 second
	os_enqueue_aperiodic_task(&change_main, OS_MILLIS(1));
}
