#!/usr/bin/env bash
set -e

# 1) Target selection & tool fallback installer
echo "Select target architecture, comma-separated choices:"
echo " 1) native (host)"
echo " 2) i686-elf (x86)"
echo " 3) x86_64-elf (AMD64)"
echo " 4) arm-none-eabi (ARM)"
echo " 5) aarch64-linux-gnu (ARM64)"
echo " 6) riscv64-unknown-elf (RISC-V)"
read -p "Enter choice [1-6]: " arch_choice

case "$arch_choice" in
  1) CC=gcc;                    LD=ld;                     FALLBACK_PKG="build-essential" ;;
  2) CC=i686-elf-gcc;           LD=i686-elf-ld;           FALLBACK_PKG="binutils-i686-elf gcc-i686-elf" ;;
  3) CC=x86_64-elf-gcc;         LD=x86_64-elf-ld;         FALLBACK_PKG="binutils-x86-64-elf gcc-x86-64-elf" ;;
  4) CC=arm-none-eabi-gcc;      LD=arm-none-eabi-ld;      FALLBACK_PKG="binutils-arm-none-eabi gcc-arm-none-eabi" ;;
  5) CC=aarch64-linux-gnu-gcc;  LD=aarch64-linux-gnu-ld;  FALLBACK_PKG="binutils-aarch64-linux-gnu gcc-aarch64-linux-gnu" ;;
  6) CC=riscv64-unknown-elf-gcc;LD=riscv64-unknown-elf-ld;FALLBACK_PKG="binutils-riscv64-unknown-elf gcc-riscv64-unknown-elf" ;;
  *) echo "Invalid choice, bro."; exit 1 ;;
esac

# install compiler if missing
if ! command -v "$CC" &>/dev/null; then
  echo "$CC not found, installing packages: $FALLBACK_PKG"
  if command -v apt-get &>/dev/null; then
    sudo apt-get update
    sudo apt-get install -y $FALLBACK_PKG
  else
    echo "No apt-get, please install: $FALLBACK_PKG"
    exit 1
  fi
fi

# static tools
NASM=nasm
GRUB=grub-mkrescue
QEMU=qemu-system-i386

# ensure nasm present
if ! command -v "$NASM" &>/dev/null; then
  echo "nasm missing, installing..."
  sudo apt-get install -y nasm || { echo "Install nasm manually"; exit 1; }
fi

# 2) Clean generated artifacts
rm -f arch/x86/boot.o \
      kernel/main.o kernel/mem.o kernel/console.o \
      kernel.bin exocore.iso
rm -rf isodir run/*.o run/*.elf run/*.bin run/linkdep_objs run/linkdep.a

# ensure dirs
mkdir -p linkdep run/linkdep_objs

# 3) auto-stub console_getc if you don’t have it
if [ ! -f linkdep/console_getc.c ]; then
  cat > linkdep/console_getc.c << 'EOF'
// auto-generated stub for console_getc
#include "console.h"
char console_getc(void) {
    return 0;  // always return zero, override as needed
}
EOF
  echo "Generated linkdep/console_getc.c stub"
fi

# 4) Compile linkdep/*.c → run/linkdep_objs/*.o
for src in linkdep/*.c; do
  [ -f "$src" ] || continue
  obj="run/linkdep_objs/$(basename "${src%.c}.o")"
  echo "Compiling linkdep $src → $obj"
  $CC -m32 -std=gnu99 -ffreestanding -O2 -nostdlib -nodefaultlibs \
      -Iinclude -c "$src" -o "$obj"
done

# archive deps so ld only pulls used ones
shopt -s nullglob
DEP_OBJS=( run/linkdep_objs/*.o )
if [ ${#DEP_OBJS[@]} -gt 0 ]; then
  echo "Creating static archive run/linkdep.a"
  ar rcs run/linkdep.a "${DEP_OBJS[@]}"
fi
shopt -u nullglob

# 5) Build console stub for modules
mkdir -p run
echo "Building console stub → run/console_mod.o"
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall \
    -Iinclude \
    -c kernel/console.c -o run/console_mod.o

# 6) Compile & link each run/*.c → .elf
for src in run/*.c; do
  [ -f "$src" ] || continue
  base=$(basename "${src%.c}")
  obj="run/${base}.o"
  elf="run/${base}.elf"

  echo "Compiling module $src → $obj"
  $CC -m32 -std=gnu99 -ffreestanding -O2 -nostdlib -nodefaultlibs \
      -Iinclude -c "$src" -o "$obj"

  echo "Linking $obj + console stub + linkdep.a → $elf"
  $LD -m elf_i386 -Ttext 0x00110000 \
      "$obj" run/console_mod.o ${DEP_OBJS:+run/linkdep.a} \
      -o "$elf"
done

# 7) Assemble any NASM .asm → .bin modules
for asm in run/*.asm; do
  [ -f "$asm" ] || continue
  bin="run/$(basename "${asm%.asm}.bin")"
  echo "Assembling $asm → $bin"
  $NASM -f bin "$asm" -o "$bin"
done

# 8) Compile & assemble the kernel
echo "Compiling kernel..."
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c arch/x86/boot.S   -o arch/x86/boot.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/main.c    -o kernel/main.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/mem.c     -o kernel/mem.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/console.c -o kernel/console.o

# 9) Link into flat kernel.bin
echo "Linking kernel.bin..."
$LD -m elf_i386 -T linker.ld \
    arch/x86/boot.o \
    kernel/main.o kernel/mem.o kernel/console.o \
    -o kernel.bin

# 10) Prepare ISO tree
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/

# 11) Copy modules into ISO
MODULES=()
for m in run/*.{bin,elf}; do
  [ -f "$m" ] || continue
  bn=$(basename "$m")
  cp "$m" isodir/boot/"$bn"
  MODULES+=( "$bn" )
done

# 12) Generate grub.cfg
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

# 13) Build the ISO
echo "Building ISO (modules: ${MODULES[*]})..."
$GRUB -o exocore.iso isodir

# 14) Run in QEMU if requested
if [ "$1" = "run" ]; then
  echo "Booting in QEMU…"
  $QEMU -cdrom exocore.iso \
       -boot order=d \
       -serial stdio \
       -monitor none
else
  echo "Done, use './build.sh run' to build & boot with serial debug"
fi

