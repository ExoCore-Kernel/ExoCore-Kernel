#!/usr/bin/env bash
set -e

# 1) compile/assemble
gcc -std=gnu99 -ffreestanding -O2 -Wall -m32 \
    -c arch/x86/boot.S -o arch/x86/boot.o
gcc -std=gnu99 -ffreestanding -O2 -Wall -m32 \
    -c kernel/main.c  -o kernel/main.o

# 2) link
ld -m elf_i386 -T linker.ld \
    arch/x86/boot.o kernel/main.o -o kernel.bin

# 3) prepare ISO dir
rm -rf isodir
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/

# 4) write grub config
cat > isodir/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin
  boot
}
EOF

# 5) detect GRUB platform dir
GRUB_MKRESCUE_BIN=$(command -v grub-mkrescue)
GRUB_PLATFORM_DIR=$(dirname "$GRUB_MKRESCUE_BIN")/../lib/grub/i386-pc
if [ ! -d "$GRUB_PLATFORM_DIR" ]; then
  echo "Warning: GRUB modules dir not found at $GRUB_PLATFORM_DIR"
  echo "Attempting default grub-mkrescue lookup..."
  # we’ll let grub-mkrescue try its defaults
  grub-mkrescue -o exocore.iso isodir
else
  # 6) build ISO with explicit GRUB modules dir
  grub-mkrescue -d "$GRUB_PLATFORM_DIR" -o exocore.iso isodir
fi

echo "==> Build complete: kernel.bin, exocore.iso"

# 7) run in QEMU if requested
if [ "$1" = "run" ] ; then
  qemu-system-i386 -cdrom exocore.iso -boot order=d
fi

