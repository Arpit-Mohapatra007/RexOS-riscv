.section .text
.global _set_ssie
.global _off_ssie
.global _set_seie
.global _enable_timer
.global _set_mie

_set_seie:
	li t0, 0x2
	csrs sstatus, t0
	li t0, 0x200
	csrs sie, t0
	ret

_set_ssie:
	li t0, 0x2
	csrs sstatus, t0
	csrs sie, t0
	ret

_off_ssie:
	li t0, 0x2
	csrc sstatus, t0
	csrc sie, t0
	ret

_enable_timer:
	ecall
	ret

_set_mie:
	li t0, 128
	csrs mie, t0
	csrr t0, mepc
	addi t0, t0, 4
	csrw mepc, t0
	ret
