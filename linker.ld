MEMORY {
	flash(RX)  : ORIGIN = 0x08000000, LENGTH = 128K
	sram(WAIL) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS {
	.text : { *(.text*) } > flash
	.bss  : {
		_sbss = .;
		*(.bss*)
		_ebss = .;
	} > sram
}
