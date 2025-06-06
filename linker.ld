ENTRY(start)
PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}
SECTIONS {
  .text 0x100000 : AT(0x100000) {
    KEEP(*(.multiboot))
    *(.text)
  } :text
  .rodata : { *(.rodata*) } :text
  .data : { *(.data) } :data
  .bss  : { *(.bss) } :data
  end = .;
}
