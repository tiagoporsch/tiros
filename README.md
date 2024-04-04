# MiROS STM32F103C8T6

This project contains a modified version of MiROS and a sample program (`main.c`) that shows an example of the producer/consumer problem, and how it can be solved with semaphores.

You need `make`, `arm-none-eabi-gcc`, and `openocd` to build and flash this project. The Makefile has a `monitor` target, that by defaults uses GNU Screen to monitor the serial port.
