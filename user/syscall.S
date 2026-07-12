.section .text
.global _sys_write
.global _sys_read
.global _sys_exit
.global _sys_getpid
.global _sys_yield
.global _sys_sleep
.global _sys_wait
.global _sys_send_msg
.global _sys_recv_msg
.global _sys_grant_permit
.global _sys_revoke_permit
.global _sys_process_count
.global _sys_process_list
.global _sys_update_whitelist
.global _sys_map_shared_page
.global _sys_unmap_shared_page
.global _sys_register_irq
.global _sys_probe_vaddr

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

_sys_send_msg:
	li a7, 8
	ecall
	ret

_sys_recv_msg:
	li a7, 9
	ecall
	ret

_sys_grant_permit:
	li a7, 10
	ecall
	ret

_sys_revoke_permit:
	li a7, 11
	ecall
	ret

_sys_process_count:
	li a7, 12
	ecall
	ret

_sys_process_list:
	li a7, 13
	ecall
	ret

_sys_update_whitelist:
	li a7, 14
	ecall
	ret

_sys_map_shared_page:
	li a7, 15
	ecall
	ret

_sys_unmap_shared_page:
	li a7, 16
	ecall
	ret

_sys_register_irq:
	li a7, 17
	ecall
	ret

_sys_probe_vaddr:
	li a7, 18
	ecall
	ret
