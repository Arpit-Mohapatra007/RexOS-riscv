.section .text
.global _set_ssie

_set_ssie:
	li t0, 0x2
	csrs sstatus, t0
	csrs sie, t0
	ret
