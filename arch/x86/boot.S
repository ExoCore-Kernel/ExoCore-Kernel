.section .multiboot
.align 4
.long 0x1BADB002       /* magic */
.long 1                /* flags: must set bit0 only */
.long -(0x1BADB002+1)  /* checksum */

.section .text
.global start
start:
	cli
	pushl %ebx           /* mbi pointer */
	pushl %eax           /* magic */
	call kernel_main

hang:
	hlt
	jmp hang
