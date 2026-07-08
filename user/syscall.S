.section .text
.global _sys_write
.global _sys_read

_sys_write:
	li a7, 1
	ecall
	ret

_sys_read:
	li a7, 2
	ecall
	ret
