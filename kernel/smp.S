.global get_runqueue
.global spinlock_acquire
.global spinlock_release


get_runqueue:
	mv a0, tp
	ret

spinlock_acquire:
	li t1, 1
lock:
	amoswap.d.aq t0, t1, (a0)
	bnez t0, lock
	ret

spinlock_release:
	amoswap.d.rl x0, x0, (a0)
	ret
