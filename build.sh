#!/usr/bin/env bash
set -e

# === CLEAN ONLY COMPILED FILES ===
rm -f arch/x86/boot.o kernel/main.o kernel.bin exocore.iso
rm -rf isodir

# === 0) Assemble run/*.asm into run/*.bin ===
mkdir -p run
for asm in run/*.asm; do
  [ -f "$asm" ] || continue
  bin="run/$(basename "${asm%.asm}.bin")"
  echo "Assembling $asm -> $bin"
  nasm -f bin "$asm" -o "$bin"
done

# === 1) Compile & assemble kernel ===
gcc -std=gnu99 -ffreestanding -O2 -Wall -m32 \
    -c arch/x86/boot.S   -o arch/x86/boot.o
gcc -std=gnu99 -ffreestanding -O2 -Wall -m32 -Iinclude \
    -c kernel/main.c    -o kernel/main.o

# === 2) Link ===
ld -m elf_i386 -T linker.ld \
   arch/x86/boot.o kernel/main.o \
   -o kernel.bin

# === 3) Prepare ISO tree ===
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/

# === 4) Copy only .bin modules from run/ ===
MODULE_ARGS=""
for bin in run/*.bin; do
  [ -f "$bin" ] || continue
  bn=$(basename "$bin")
  cp "$bin" isodir/boot/"$bn"
  MODULE_ARGS="$MODULE_ARGS /boot/$bn"
done

# === 5) Write GRUB menu (timeout 5s) ===
cat > isodir/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin${MODULE_ARGS}
  boot
}
EOF

# === 6) Build the ISO ===
grub-mkrescue -o exocore.iso isodir
echo "==> Built: kernel.bin + exocore.iso  modules:${MODULE_ARGS}"

# === 7) Only run if asked ===
if [ "$1" = "run" ]; then
  qemu-system-i386 -cdrom exocore.iso -boot order=d
else
  echo "Use './build.sh run' to boot in QEMU graphically"
fi
