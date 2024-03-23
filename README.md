# MiROS STM32F103C8T6

This project contains a modified version of MiROS and a sample program (`main.c`) that shows basic usage of cooperation features like semaphores and yield commands.

You need `make`, `arm-none-eabi-gcc`, and `openocd` to build and flash this project. The Makefile has a `monitor` target, that by defaults uses GNU Screen to monitor the serial port.
