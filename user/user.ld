ENTRY(main)

SECTIONS {
	. = 0x00000000;

	.text : {
		*(.text*)
		. = ALIGN(0x1000);
	}
	
	.rodata : {
		*(.rodata*)
		. = ALIGN(0x1000);
	}
	
	.data : {
		*(.data*)
		. = ALIGN(0x1000);
	}

	.bss : {
		*(.bss* COMMON)
		. = ALIGN(0x1000);
	}
}
