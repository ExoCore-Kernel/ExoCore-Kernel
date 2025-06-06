ENTRY(start)

SECTIONS {
  /* The kernel is loaded at 1 MiB, but the multiboot header must
     appear within the first 8 KiB of the file.  We therefore load
     sections starting at the 1 MiB virtual address while placing the
     first bytes of the output file at offset 0.  */

  /* Virtual address where the kernel expects to be loaded.  */
  . = 0x100000;

  /* Text section also contains the multiboot header so that GRUB
     finds it within the first few KiB of the file. */
  .text ALIGN(4) : AT(0x100000) {
    KEEP(*(.multiboot))
    *(.text*)
  }

  /* Remaining sections are relocated so that their file offsets are
     1 MiB lower than their load addresses. */

  .rodata ALIGN(4) : AT(ADDR(.rodata) - 0x100000) {
    *(.rodata*)
  }

  .data ALIGN(4) : AT(ADDR(.data) - 0x100000) {
    *(.data*)
  }

  .bss ALIGN(4) : AT(ADDR(.bss) - 0x100000) {
    *(.bss*)
  }

  end = .;
}
