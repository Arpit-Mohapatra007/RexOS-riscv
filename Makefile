TARGET = kernel.elf

CC = riscv64-unknown-elf-gcc

OBJCOPY = riscv64-unknown-elf-objcopy

QEMU = qemu-system-riscv64

CFLAGS = -nostdlib \
	 -Wall \
	 -march=rv64gc \
	 -mabi=lp64d \
	 -fno-builtin \
	 -ffreestanding \
	 -mcmodel=medany \
	 -msmall-data-limit=0 \
	 -Wextra \
	 -g \
	 -Ikernel

BFLAGS = -machine virt \
	 -cpu rv64 \
	 -smp 8 \
	 -m 128M \
	 -bios none \
	 -kernel

SRCS = kernel/start.S \
       kernel/uart.c \
       kernel/string.c \
       kernel/dtb.c \
       kernel/kalloc.c \
       kernel/vm.c \
       kernel/vm.S \
       kernel/timer.c \
       kernel/timer.S \
       kernel/scheduler.c \
       kernel/scheduler.S \
       kernel/main.c \
       user/shell_payload.o \
       user/worker_payload.o

USRCS = user/syscall.S \
       user/util.c \
       user/shell.c

WSRCS = user/syscall.S \
	user/util.c \
	user/worker.c

all: $(TARGET)

$(TARGET) : $(SRCS) linker.ld
	$(CC) $(CFLAGS) -T linker.ld $(SRCS) -o $(TARGET)

run: $(TARGET)
	$(QEMU) $(BFLAGS) $(TARGET) -nographic

user/shell_payload.o: user/shell.elf
	$(OBJCOPY) -I binary -O elf64-littleriscv user/shell.elf user/shell_payload.o

user/shell.elf: $(USRCS) user.ld
	$(CC) $(CFLAGS) -T user.ld $(USRCS) -o user/shell.elf

user/worker_payload.o: user/worker.elf
	$(OBJCOPY) -I binary -O elf64-littleriscv user/worker.elf user/worker_payload.o

user/shell.elf: $(WSRCS) user/user.ld
	$(CC) $(CFLAGS) -T user/user.ld $(WSRCS) -o user/worker.elf


clean:
	rm -f $(TARGET) user/*.o user/*.elf kernel/*.o kernel/*.elf

.PHONY: all run extract clean
