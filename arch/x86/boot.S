    .section .multiboot
    .align 4
    .long 0x1BADB002           # magic
    .long 0                     # flags
    .long -(0x1BADB002)         # checksum

    .section .text
    .global start
start:
    cli                         # clear interrupts
    call kernel_main            # jump into C code
1:
    hlt                         # halt CPU
    jmp 1b                      # loop here forever
