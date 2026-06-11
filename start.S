.text
.global _start
_start:
	la sp, _stack_top
	la t0, _bss_start
	la t1, _bss_end

_bss_clear:
	bge t0, t1, _done
	sd x0, 0(t0)
	addi t0, t0, 8
	j _bss_clear

_done:
	call kmain
	wfi
	j _done

.bss

.align 12

_stack_bottom:
	.skip 4096
_stack_top:
