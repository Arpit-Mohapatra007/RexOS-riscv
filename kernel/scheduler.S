.section .text
.global _load_sscratch
.global _off_sip

_load_sscratch:
	csrw sscratch, a0
	ret

_off_sip:
	li t0, 0x2
	csrc sip, t0
	ret
