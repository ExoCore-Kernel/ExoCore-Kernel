#!/usr/bin/env bash
set -e

# --- 0) Assemble run/*.asm into run/*.bin ---
mkdir -p run
for asm in run/*.asm; do
  [ -f "$asm" ] || continue
  bin="run/$(basename "${asm%.asm}.bin")"
  echo "Assembling $asm -> $bin"
  nasm -f bin "$asm" -o "$bin"
done

# --- 1) Compile/assemble kernel ---
gcc -std=gnu99 -ffreestanding -O2 -Wall -m32 \
    -c arch/x86/boot.S   -o arch/x86/boot.o
gcc -std=gnu99 -ffreestanding -O2 -Wall -m32 \
    -c kernel/main.c    -o kernel/main.o

# --- 2) Link ---
ld -m elf_i386 -T linker.ld \
   arch/x86/boot.o kernel/main.o \
   -o kernel.bin

# --- 3) Prepare ISO tree ---
rm -rf isodir
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/

# --- 4) Copy run/ modules ---
MODULE_ARGS=""
for f in run/*; do
  [ -f "$f" ] || continue
  bn=$(basename "$f")
  cp "$f" isodir/boot/"$bn"
  MODULE_ARGS="$MODULE_ARGS /boot/$bn"
done

# --- 5) Write GRUB config ---
cat > isodir/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin${MODULE_ARGS}
  boot
}
EOF

# --- 6) Build ISO ---
grub-mkrescue -o exocore.iso isodir
echo "==> Build complete: kernel.bin, exocore.iso with modules:${MODULE_ARGS}"

# --- 7) Boot in QEMU (curses text mode) ---
qemu-system-i386 -cdrom exocore.iso -boot order=d -display curses

