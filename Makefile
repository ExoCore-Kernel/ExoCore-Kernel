# Toolchain
CC      = gcc
AS      = gcc
LD      = ld

# Build flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -m32
LDFLAGS = -m elf_i386 -T linker.ld

# ISO settings
ISO_DIR = isodir
ISO     = exocore.iso

.PHONY: all clean run

# Default target builds the ISO (which depends on kernel.bin)
all: $(ISO)

# Build the kernel image
kernel.bin: arch/x86/boot.o kernel/main.o
	$(LD) $(LDFLAGS) -o $@ $^

# Assemble and compile sources
arch/x86/boot.o: arch/x86/boot.S
	$(AS) $(CFLAGS) -c $< -o $@

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create the bootable ISO
$(ISO): kernel.bin
	cp kernel.bin $(ISO_DIR)/boot/
	grub-mkrescue -o $(ISO) $(ISO_DIR)

# Launch in QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

# Clean up all outputs
clean:
	rm -f arch/x86/*.o kernel/*.o kernel.bin $(ISO)

