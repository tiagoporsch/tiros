_estack = ORIGIN(SRAM) + LENGTH(SRAM);

MEMORY {
	FLASH (RX) : ORIGIN = 0x08000000, LENGTH = 64K
	SRAM (RWX) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS {
	.text : {
		*(.isr_vector)
		*(.text*)
	} > FLASH
	.data : {
		_sdata = .;
		*(.data)
		*(.data*)
		_edata = .;
	} > SRAM AT > FLASH
	.bss : {
		_sbss = .;
		*(.bss*)
		_ebss = .;
	} > SRAM
}
