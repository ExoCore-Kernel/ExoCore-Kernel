.section .multiboot
.align 4
.long 0x1BADB002    /* magic */
.long 0             /* flags: plain Multiboot v1 */
.long -(0x1BADB002) /* checksum */

.section .text
.global start
start:
	cli
	pushl %ebx       /* mbi ptr */
	pushl %eax       /* magic */
	call kernel_main

hang:
	hlt
	jmp hang
