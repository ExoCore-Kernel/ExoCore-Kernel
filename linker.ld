ENTRY(start)
SECTIONS {
  . = 1M;                    /* load at 1 MiB */
  .multiboot : { KEEP(*(.multiboot)) }
  .text       : { *(.text)     . = ALIGN(4); }
  .rodata     : { *(.rodata*)  . = ALIGN(4); }
  .data       : { *(.data)     . = ALIGN(4); }
  .bss        : { *(.bss)      . = ALIGN(4); }
  end = .;
}
