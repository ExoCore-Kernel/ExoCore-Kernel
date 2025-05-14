#!/usr/bin/env bash
set -e

# === Configuration: adjust if your cross‐tools live elsewhere ===
CC=i686-elf-gcc
LD=i686-elf-ld
NASM=nasm
GRUB=grub-mkrescue
QEMU=qemu-system-i386

# === 0) Clean generated artifacts ===
rm -f arch/x86/boot.o \
      kernel/main.o kernel/mem.o kernel/console.o \
      kernel.bin exocore.iso
rm -rf isodir run/*.o run/*.elf run/*.bin

# === 1) Build a console stub for modules ===
mkdir -p run
echo "Building console stub → run/console_mod.o"
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/console.c -o run/console_mod.o

# === 2) Compile each C app in run/ → ELF modules ===
for src in run/*.c; do
  [ -f "$src" ] || continue
  base=$(basename "${src%.c}")
  obj="run/${base}.o"
  elf="run/${base}.elf"
  echo "Compiling module $src → $obj"
  $CC -m32 -std=gnu99 -ffreestanding -O2 -nostdlib -nodefaultlibs \
      -Iinclude -c "$src" -o "$obj"
  echo "Linking module $obj + console stub → $elf"
  $LD -m elf_i386 -Ttext 0x00110000 \
      "$obj" run/console_mod.o \
      -o "$elf"
done

# === 3) Assemble any NASM .asm → .bin modules ===
for asm in run/*.asm; do
  [ -f "$asm" ] || continue
  bin="run/$(basename "${asm%.asm}.bin")"
  echo "Assembling $asm → $bin"
  $NASM -f bin "$asm" -o "$bin"
done

# === 4) Compile & assemble the kernel ===
echo "Compiling kernel sources..."
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c arch/x86/boot.S       -o arch/x86/boot.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/main.c        -o kernel/main.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/mem.c         -o kernel/mem.o
$CC -m32 -std=gnu99 -ffreestanding -O2 -Wall -Iinclude \
    -c kernel/console.c     -o kernel/console.o

# === 5) Link into flat kernel.bin ===
echo "Linking kernel.bin..."
$LD -m elf_i386 -T linker.ld \
    arch/x86/boot.o \
    kernel/main.o kernel/mem.o kernel/console.o \
    -o kernel.bin

# === 6) Prepare the ISO tree ===
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/

# === 7) Copy modules (.bin + .elf) into the ISO ===
MODULES=()
for m in run/*.{bin,elf}; do
  [ -f "$m" ] || continue
  bn=$(basename "$m")
  cp "$m" isodir/boot/"$bn"
  MODULES+=( "$bn" )
done

# === 8) Generate grub.cfg with module directives ===
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

# === 9) Build the ISO ===
echo "Building ISO (modules: ${MODULES[*]})..."
$GRUB -o exocore.iso isodir

# === 10) Run in QEMU if requested ===
if [ "$1" = "run" ]; then
  echo "Booting in QEMU..."
  $QEMU -cdrom exocore.iso \
       -boot order=d \
       -serial stdio \
       -monitor none
else
  echo "Done. Use './build.sh run' to build & boot with serial debug"
fi
