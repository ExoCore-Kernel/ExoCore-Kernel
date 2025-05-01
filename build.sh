#!/usr/bin/env bash
set -e

# --- CONFIG: adjust if your cross-tools live elsewhere ---
CC=i686-elf-gcc
LD=i686-elf-ld
NASM=nasm
GRUB=grub-mkrescue
QEMU=qemu-system-i386

# 0) Clean generated artifacts
rm -f arch/x86/boot.o \
      kernel/main.o kernel/mem.o kernel/console.o \
      kernel.bin exocore.iso
rm -rf isodir

# 1) Compile C apps in run/ → ELF
mkdir -p run
for src in run/*.c; do
  [ -f "$src" ] || continue
  elf="run/$(basename "${src%.c}.elf")"
  echo "Compiling $src → $elf"
  $CC -m32 -std=gnu99 -ffreestanding -O2 \
      -nostdlib -nodefaultlibs \
      -Iinclude -Ttext 0x00110000 \
      "$src" -o "$elf"
done

# 2) Assemble NASM .asm → .bin
for asm in run/*.asm; do
  [ -f "$asm" ] || continue
  bin="run/$(basename "${asm%.asm}.bin")"
  echo "Assembling $asm → $bin"
  $NASM -f bin "$asm" -o "$bin"
done

# 3) Compile & assemble the kernel
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c arch/x86/boot.S       -o arch/x86/boot.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/main.c        -o kernel/main.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/mem.c         -o kernel/mem.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/console.c     -o kernel/console.o

# 4) Link into flat kernel.bin
$LD -m elf_i386 -T linker.ld \
    arch/x86/boot.o \
    kernel/main.o kernel/mem.o kernel/console.o \
    -o kernel.bin

# 5) Prepare the ISO tree
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/

# 6) Copy modules (.bin + .elf)
MODULES=()
for m in run/*.{bin,elf}; do
  [ -f "$m" ] || continue
  bn=$(basename "$m")
  cp "$m" isodir/boot/"$bn"
  MODULES+=( "$bn" )
done

# 7) Write GRUB2 config
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=5
set default=0

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin
EOF
for mod in "${MODULES[@]}"; do
  echo "  module /boot/$mod" >> isodir/boot/grub/grub.cfg
done
cat >> isodir/boot/grub/grub.cfg << EOF
  boot
}
EOF

# 8) Build the ISO
$GRUB -o exocore.iso isodir
echo "Built kernel.bin + exocore.iso (modules: ${MODULES[*]})"

# 9) Launch if requested
if [ "$1" = "run" ]; then
  $QEMU -cdrom exocore.iso -boot order=d \
       -serial stdio -monitor none
else
  echo "Use './build.sh run' to build and boot with serial debug"
fi
