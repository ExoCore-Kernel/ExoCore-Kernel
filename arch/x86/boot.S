.section .multiboot
.align 4
.long 0x1BADB002       /* magic */
.long 17               /* flags: ALIGN_MODULES (1) | MODULES_INFO (16) */
.long -(0x1BADB002 + 17) /* checksum */

.section .text
.global start
start:
	cli
	pushl %ebx           /* mbi ptr */
	pushl %eax           /* magic */
	call kernel_main

hang:
	hlt
	jmp hang
