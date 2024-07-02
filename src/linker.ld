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
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
  } > FLASH
	.bss : {
		_sbss = .;
		*(.bss*)
		_ebss = .;
	} > SRAM
}
