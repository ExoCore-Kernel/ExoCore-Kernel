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

# 3a) copy kernel
cp kernel.bin isodir/boot/

# 3b) copy run/ directory files as GRUB modules
mkdir -p run      # ensure run/ exists
MODULE_ARGS=""
for f in run/*; do
  [ -f "$f" ] || continue
  bn=$(basename "$f")
  cp "$f" isodir/boot/"$bn"
  MODULE_ARGS="$MODULE_ARGS /boot/$bn"
done

# 4) write GRUB config (timeout 5s, pass in all modules)
cat > isodir/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin${MODULE_ARGS}
  boot
}
EOF

# 5) build ISO
grub-mkrescue -o exocore.iso isodir
echo "==> Build complete: kernel.bin, exocore.iso with modules:${MODULE_ARGS}"

# 6) boot in QEMU (text mode)
qemu-system-i386 -cdrom exocore.iso -boot order=d -display curses

