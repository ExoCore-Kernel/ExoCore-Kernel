.section .multiboot
.align 4
.long 0x1BADB002       /* magic */
.long 0                  /* flags */
.long -(0x1BADB002)      /* checksum */

.section .text
.global start
start:
    cli
    pushl %ebx           /* Multiboot info ptr */
    pushl %eax           /* Multiboot magic */
    call kernel_main

hang:
    hlt
    jmp hang

