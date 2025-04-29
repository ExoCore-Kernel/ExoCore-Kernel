# Toolchain
CC      = i686-elf-gcc
AS      = i686-elf-as
LD      = i686-elf-ld

# Compiler and linker flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -m32
LDFLAGS = -T linker.ld

# Targets
all: kernel.bin

arch/x86/boot.o: arch/x86/boot.S
	$(AS) arch/x86/boot.S -o arch/x86/boot.o

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) -c kernel/main.c -o kernel/main.o

kernel.bin: arch/x86/boot.o kernel/main.o
	$(LD) $(LDFLAGS) arch/x86/boot.o kernel/main.o -o kernel.bin

run: all
	qemu-system-i386 -kernel kernel.bin

clean:
	rm -f arch/x86/*.o kernel/*.o kernel.bin
