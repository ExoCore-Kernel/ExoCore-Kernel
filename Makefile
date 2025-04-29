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

# Default: build the ISO (which depends on kernel.bin)
all: $(ISO)

# Link the kernel image
kernel.bin: arch/x86/boot.o kernel/main.o
	$(LD) $(LDFLAGS) -o $@ $^

# Assemble the boot stub
arch/x86/boot.o: arch/x86/boot.S
	$(AS) $(CFLAGS) -c $< -o $@

# Compile the C entry point
kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create a bootable ISO with GRUB
$(ISO): kernel.bin
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp kernel.bin $(ISO_DIR)/boot/
	cat > $(ISO_DIR)/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin
  boot
}
EOF
	grub-mkrescue -o $(ISO) $(ISO_DIR)

# Launch the ISO in QEMU (force CD boot)
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -boot order=d

# Clean all generated files
clean:
	rm -rf arch/x86/*.o kernel/*.o kernel.bin $(ISO) $(ISO_DIR)

