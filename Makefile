PROJECT:=miros

AS:=arm-none-eabi-as
CC:=arm-none-eabi-gcc
LD:=arm-none-eabi-ld.bfd

CFLAGS:=-DOS_DEBUG_GPIO -Iinclude -MD -Wall -Wextra -fno-builtin -mcpu=cortex-m3 -mthumb -nostartfiles -nostdlib
LDFLAGS:=-Tsrc/linker.ld

OBJECTS:=$(patsubst src/%,bin/%.o,$(wildcard src/*.c src/*.s))

bin/$(PROJECT).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

bin/%.s.o: src/%.s
	@mkdir -p "$(@D)"
	$(AS) -o $@ $<

bin/%.c.o: src/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: flash monitor clean

flash: bin/$(PROJECT).elf
	openocd -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg -c "program $< verify reset exit"

monitor:
	screen /dev/ttyUSB0 115200

clean:
	rm -rf bin/

-include $(OBJECTS:.o=.d)
