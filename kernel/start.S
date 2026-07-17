.section .text
.global _start
.global _trigger_smode_software_interrupt
.global _wfi
.global _user_trampoline
.global _load_umode_stvec
.global _load_smode_stvec
.global _process_bootstrap
.global _load_tp

.extern global_closet
.extern smp_ready

_start:
	csrr a0, mhartid
	bnez a0, _hart_park
	
	la t0, _dtb_ptr
	sd a1, 0(t0)
	
	li t0, -1
	csrw pmpaddr0, t0
	li t0, 0x1F
	csrw pmpcfg0, t0

	li t0, -1
	csrw medeleg, t0
	csrw mideleg, t0	
	
	la t0, _mmode_trap_handler
	csrw mtvec, t0

	la sp, _stack_top
	la t0, _m_stack_top
	csrw mscratch, t0
	la t0, _bss_start
	la t1, _bss_end

_bss_clear:
	bge t0, t1, _done
	sd x0, 0(t0)
	addi t0, t0, 8
	j _bss_clear

_done:
	li t0, 0x1800
	csrc mstatus, t0
	li t0, 0x0800
	csrs mstatus, t0

	la t0, 0x02004000
	li t1, -1
	sd t1, 0(t0)

	li t0, 128
	csrs mie, t0

	li t0, 32
	csrs mstatus, t0
	
	la t0, kmain
	csrw mepc, t0
	
	la t0, _smode_trap_handler
	csrw stvec, t0
	
	mret
	wfi
	j _done

_hart_park:
	li t0, 8
	csrs mie, t0
	wfi

_smp_awakening_sequence:
	la t0, smp_ready
	ld t0, 0(t0)
	beqz t0, _smp_awakening_sequence

	csrr t0, mhartid
	li t1, 32
	mul t0, t0, t1
	la t2, global_closet
	ld t2, 0(t2)
	add t2, t2, t0

	ld t1, 16(t2)
	beqz t1, _smp_awakening_sequence

	ld t1, 0(t2)
	csrw mscratch, t1

	ld sp, 8(t2)

	la t0, _mmode_trap_handler
	csrw mtvec, t0

	la t0, _smode_trap_handler
	csrw stvec, t0

	la t1, root_table
	ld t1, 0(t1)
	li t0, 0x8000000000000000
	srli t1, t1, 12
	or t0, t0, t1
	csrw satp, t0
	sfence.vma
	
	li t0, -1
	csrw pmpaddr0, t0
	li t0, 0x1F
	csrw pmpcfg0, t0

	li t0, -1
	csrw medeleg, t0
	csrw mideleg, t0	

	li t0, 0x1800
	csrc mstatus, t0
	li t0, 0x0800
	csrs mstatus, t0

	la t0, 0x02004000
	csrr t2, mhartid
	li t1, 8
	mul t2, t2, t1
	add t0, t0, t2
	li t1, -1
	sd t1, 0(t0)

	li t0, 128
	csrs mie, t0

	li t0, 32
	csrs mstatus, t0

	la t1, smp_kmain
	csrw mepc, t1

	csrr t0, mhartid
	li t1, 4
	mul t0, t0, t1
	li t1, 0x02000000
	add t1, t1, t0
	sw x0, 0(t1)
	
	csrr a0, mhartid
	mret

.align 2
_mmode_trap_handler:
	csrrw sp, mscratch, sp
	addi sp, sp, -256
	sd x1, 0(sp)
	csrr x1, mscratch
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
	csrr a2, mhartid
	call mtrap_router
	ld x31, 240(sp)
	ld x30, 232(sp)
	ld x29, 224(sp)
	ld x28, 216(sp)
	ld x27, 208(sp)
	ld x26, 200(sp)
	ld x25, 192(sp)
	ld x24, 184(sp)
	ld x23, 176(sp)
	ld x22, 168(sp)
	ld x21, 160(sp)
	ld x20, 152(sp)
	ld x19, 144(sp)
	ld x18, 136(sp)
	ld x17, 128(sp)
	ld x16, 120(sp)
	ld x15, 112(sp)
	ld x14, 104(sp)
	ld x13, 96(sp)
	ld x12, 88(sp)
	ld x11, 80(sp)
	ld x10, 72(sp)
	ld x9, 64(sp)
	ld x8, 56(sp)
	ld x7, 48(sp)
	ld x6, 40(sp)
	ld x5, 32(sp)
	ld x4, 24(sp)
	ld x3, 16(sp)
	ld x1, 8(sp)
	csrw mscratch, x1
	ld x1, 0(sp)
	addi sp, sp, 256
	csrrw sp, mscratch, sp
	mret

.align 2
_smode_trap_handler:
	addi sp, sp, -272
	sd x0, 0(sp)
	sd x1, 8(sp)
	addi x1, sp, 272
	sd x1, 16(sp)
	sd x3, 24(sp)
	sd x4, 32(sp)
	sd x5, 40(sp)
	sd x6, 48(sp)
	sd x7, 56(sp)
	sd x8, 64(sp)
	sd x9, 72(sp)
	sd x10, 80(sp)
	sd x11, 88(sp)
	sd x12, 96(sp)
	sd x13, 104(sp)
	sd x14, 112(sp)
	sd x15, 120(sp)
	sd x16, 128(sp)
	sd x17, 136(sp)
	sd x18, 144(sp)
	sd x19, 152(sp)
	sd x20, 160(sp)
	sd x21, 168(sp)
	sd x22, 176(sp)
	sd x23, 184(sp)
	sd x24, 192(sp)
	sd x25, 200(sp)
	sd x26, 208(sp)
	sd x27, 216(sp)
	sd x28, 224(sp)
	sd x29, 232(sp)
	sd x30, 240(sp)
	sd x31, 248(sp)
	csrr t0, sepc
	sd t0, 256(sp)
	csrr t0, sstatus
	sd t0, 264(sp)
	csrr a0, scause
	csrr a1, stval
	call strap_router
	ld t0, 264(sp)
	csrw sstatus, t0
	ld t0, 256(sp)
	csrw sepc, t0
	ld x31, 248(sp)
	ld x30, 240(sp)
	ld x29, 232(sp)
	ld x28, 224(sp)
	ld x27, 216(sp)
	ld x26, 208(sp)
	ld x25, 200(sp)
	ld x24, 192(sp)
	ld x23, 184(sp)
	ld x22, 176(sp)
	ld x21, 168(sp)
	ld x20, 160(sp)
	ld x19, 152(sp)
	ld x18, 144(sp)
	ld x17, 136(sp)
	ld x16, 128(sp)
	ld x15, 120(sp)
	ld x14, 112(sp)
	ld x13, 104(sp)
	ld x12, 96(sp)
	ld x11, 88(sp)
	ld x10, 80(sp)
	ld x9, 72(sp)
	ld x8, 64(sp)
	ld x7, 56(sp)
	ld x6, 48(sp)
	ld x5, 40(sp)
	ld x4, 32(sp)
	ld x3, 24(sp)
	ld x1, 8(sp)
	ld x0, 0(sp)
	addi sp, sp, 272
	sret

.align 12
_user_trampoline:
	csrrw sp, sscratch, sp
	sd x0, 0(sp)
	sd x1, 8(sp)
	csrr x1, sscratch
	sd x1, 16(sp)
	sd x3, 24(sp)
	sd x4, 32(sp)
	sd x5, 40(sp)
	sd x6, 48(sp)
	sd x7, 56(sp)
	sd x8, 64(sp)
	sd x9, 72(sp)
	sd x10, 80(sp)
	sd x11, 88(sp)
	sd x12, 96(sp)
	sd x13, 104(sp)
	sd x14, 112(sp)
	sd x15, 120(sp)
	sd x16, 128(sp)
	sd x17, 136(sp)
	sd x18, 144(sp)
	sd x19, 152(sp)
	sd x20, 160(sp)
	sd x21, 168(sp)
	sd x22, 176(sp)
	sd x23, 184(sp)
	sd x24, 192(sp)
	sd x25, 200(sp)
	sd x26, 208(sp)
	sd x27, 216(sp)
	sd x28, 224(sp)
	sd x29, 232(sp)
	sd x30, 240(sp)
	sd x31, 248(sp)
	csrr t0, sepc
	sd t0, 256(sp)
	csrr t0, sstatus
	sd t0, 264(sp)
	ld t0, 272(sp)
	ld t1, 280(sp)
	ld t2, 288(sp)
	ld tp, 304(sp)
	csrw satp, t0
	sfence.vma
	csrw sscratch, sp
	mv sp, t1
	la t0, _smode_trap_handler
	csrw stvec, t0
	csrr a0, scause
	csrr a1, stval
	jalr ra, t2, 0
	csrr sp, sscratch
_restore_state:
	li t0, 0x3FFFFFF000
	csrw stvec, t0
	ld t0, 264(sp)
	csrw sstatus, t0
	ld t0, 256(sp)
	csrw sepc, t0
	ld t0, 296(sp)
	li t1, 0x3FFFFFEEC8
	csrw satp, t0
	sfence.vma
	mv sp, t1
	ld x31, 248(sp)
	ld x30, 240(sp)
	ld x29, 232(sp)
	ld x28, 224(sp)
	ld x27, 216(sp)
	ld x26, 208(sp)
	ld x25, 200(sp)
	ld x24, 192(sp)
	ld x23, 184(sp)
	ld x22, 176(sp)
	ld x21, 168(sp)
	ld x20, 160(sp)
	ld x19, 152(sp)
	ld x18, 144(sp)
	ld x17, 136(sp)
	ld x16, 128(sp)
	ld x15, 120(sp)
	ld x14, 112(sp)
	ld x13, 104(sp)
	ld x12, 96(sp)
	ld x11, 88(sp)
	ld x10, 80(sp)
	ld x9, 72(sp)
	ld x8, 64(sp)
	ld x7, 56(sp)
	ld x6, 48(sp)
	ld x5, 40(sp)
	ld x4, 32(sp)
       	ld x3, 24(sp)
	ld x1, 8(sp)
	ld x0, 0(sp)
	csrw sscratch, sp
	ld x2, 16(sp)
	sret
 
_process_bootstrap:
	la t0, _user_trampoline
	la t1, _restore_state
	sub t1, t1, t0
	li t0, 0x3FFFFFF000
	add t0, t0, t1
	jr t0

_load_tp:
	mv tp, a0
	ret

_load_umode_stvec:
	li t0, 0x3FFFFFF000
	csrw stvec, t0
	ret

_load_smode_stvec:
	la t0, _smode_trap_handler
	csrw stvec, t0
	ret

_trigger_smode_software_interrupt:
	li t0, 0x2
	csrs sip, t0
	ret

_trap_handler_done:
	wfi
	j _trap_handler_done

_wfi:
	wfi
	ret

.section .data
.global _dtb_ptr
.global _stack_top

_dtb_ptr:
	.quad 0

.section .bss

.align 12
_m_stack_bottom:
	.skip 4096
_m_stack_top:

_stack_bottom:
	.skip 8192
_stack_top:
