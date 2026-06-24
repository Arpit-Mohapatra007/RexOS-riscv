.section .text
.global _load_satp

_load_satp:
	srli t1, a0, 12
	li t2, 8
	slli t2, t2, 60
	or t2, t2, t1
	sfence.vma
	csrw satp, t2
	sfence.vma
	fence.i
	ret
