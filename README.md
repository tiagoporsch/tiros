# MiROS STM32F103C8T6

This project contains a heavily modified version of MiROS that uses an EDF (Earliest Deadline First) scheduler. It has support for periodic tasks with deadlines that differ from the period. Support for non-periodic tasks is a work in progress; right now it's possible to create a thread that executes only once by setting its period to zero.

The working principle is the global tick counter `os_ticks` and the `activation_time` variable contained in each task's TCB (Thread Control Block). If the `activation_time` is in the future, the task has not yet been activated; if it's in the past, the thread is active and its absolute deadline can be calculated by adding `relative_deadline` to the `activation_time`.

Having each active thread's absolute deadline, when the scheduler is called, it searches for the earliest one, and switches to it. Upon terminating execution, the task `period` is added to its `activation_time`.

## Compiling

You need `make`, `arm-none-eabi-gcc`, and `openocd` to build and flash this project. The Makefile has a `monitor` target, that by defaults uses GNU Screen to monitor the serial port.
