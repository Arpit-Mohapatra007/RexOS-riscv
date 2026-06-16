ENTRY(_start)

SECTIONS {
	. = 0x80000000;

	.text : {
		_text_start = .;
		*(.text*)
		. = ALIGN(0x1000);
		_text_end = .;
	}

	. = ALIGN(0x1000);
	
	.rodata : {
		_rodata_start = .;
		*(.rodata*)
		. = ALIGN(0x1000);
		_rodata_end = .;
	}

	. = ALIGN(0x1000);
	
	.data : {
		_data_start = .;
		*(.data*)
		. = ALIGN(0x1000);
		_data_end = .;
	}

	. = ALIGN(0x1000);
	
	.bss : {
		_bss_start = .;
		*(.bss* COMMON)
		. = ALIGN(0x1000);
		_bss_end = .;
	}
	
	_kernel_end = .;
}
