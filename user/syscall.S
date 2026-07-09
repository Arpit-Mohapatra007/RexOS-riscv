.section .text
.global _sys_write
.global _sys_read
.global _sys_exit
.global _sys_getpid
.global _sys_yield
.global _sys_sleep
.global _sys_wait

_sys_write:
	li a7, 1
	ecall
	ret

_sys_read:
	li a7, 2
	ecall
	ret

_sys_exit:
	li a7, 3
	ecall
	ret

_sys_getpid:
	li a7, 4
	ecall
	ret

_sys_yield:
	li a7, 5
	ecall
	ret

_sys_sleep:
	li a7, 6
	ecall
	ret

_sys_wait:
	li a7, 7
	ecall
	ret
