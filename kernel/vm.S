.section .text
.global _load_satp

_load_satp:
	la t0, root_table
	ld t1, 0(t0)
	srli t1, t1, 12
	li t2, 8
	slli t2, t2, 60
	or t2, t2, t1
	sfence.vma
	csrw satp, t2
	sfence.vma
	fence.i
	ret
