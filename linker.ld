ENTRY(_start)

SECTIONS {
	. = 0x80000000;

	.text : {
		*(.text*)
	}

	. = ALIGN(0x1000);

	.rodata : {
		*(.rodata*)
	}

	. = ALIGN(0x1000);

	.data : {
		*(.data*)
	}

	. = ALIGN(0x1000);

	.bss : {
		_bss_start = .;
		*(.bss* COMMON)
		_bss_end = .;
	}

	_kernel_end = .;
}
