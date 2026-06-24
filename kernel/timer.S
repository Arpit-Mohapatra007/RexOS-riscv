.section .text
.global _set_ssie
.global _off_ssie
.global _set_seie

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
