TARGET = kernel.elf

CC = riscv64-unknown-elf-gcc

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
       kernel/main.c

all: $(TARGET)

$(TARGET) : $(SRCS) linker.ld
	$(CC) $(CFLAGS) -T linker.ld $(SRCS) -o $(TARGET)

run: $(TARGET)
	$(QEMU) $(BFLAGS) $(TARGET) -nographic

clean:
	rm -f $(TARGET)

.PHONY: all run clean
