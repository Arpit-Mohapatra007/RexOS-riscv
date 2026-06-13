.section .text
.global _start
_start:
	la t0, _dtb_ptr
	sd a1, 0(t0)	
	csrr a0, mhartid
	bnez a0, _hart_park
	la t0, _trap_handler
	csrw mtvec, t0
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

_hart_park:
	wfi
	j _hart_park

_trap_handler:
	addi sp, sp, -256
	sd x1, 0(sp)
	addi x1, x2, 256
	sd x1, 8(sp)
	sd x3, 16(sp)
	sd x4, 24(sp)
	sd x5, 32(sp)
	sd x6, 40(sp)
	sd x7, 48(sp)
	sd x8, 56(sp)
	sd x9, 64(sp)
	sd x10, 72(sp)
	sd x11, 80(sp)
	sd x12, 88(sp)
	sd x13, 96(sp)
	sd x14, 104(sp)
	sd x15, 112(sp)
	sd x16, 120(sp)
	sd x17, 128(sp)
	sd x18, 136(sp)
	sd x19, 144(sp)
	sd x20, 152(sp)
	sd x21, 160(sp)
	sd x22, 168(sp)
	sd x23, 176(sp)
	sd x24, 184(sp)
	sd x25, 192(sp)
	sd x26, 200(sp)
	sd x27, 208(sp)
	sd x28, 216(sp)
	sd x29, 224(sp)
	sd x30, 232(sp)
	sd x31, 240(sp)
	csrr a0, mcause
	csrr a1, mepc
	call kpanic

_trap_handler_done:
	wfi
	j _trap_handler_done

.section .data
.global _dtb_ptr
_dtb_ptr:
	.quad 0

.section .bss

.align 12

_stack_bottom:
	.skip 8192
_stack_top:
