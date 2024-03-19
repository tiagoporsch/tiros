PROJECT:=miros

AS:=arm-none-eabi-as
CC:=arm-none-eabi-gcc
LD:=arm-none-eabi-ld.bfd

CFLAGS:=-Iinc -mcpu=cortex-m3 -mthumb
LDFLAGS:=-Tlinker.ld

OBJECTS:=bin/startup.s.o $(patsubst src/%,bin/%.o,$(wildcard src/*.c))

bin/$(PROJECT).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

bin/%.s.o: src/%.s
	@mkdir -p "$(@D)"
	$(AS) -o $@ $<

bin/%.c.o: src/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c -o $@ $<

flash: bin/$(PROJECT).elf
	openocd -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg -c "program $< verify reset exit"

monitor:
	screen /dev/ttyUSB0 115200

clean:
	rm -fr bin/

.PHONY: flash monitor clean
