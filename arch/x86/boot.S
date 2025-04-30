.section .multiboot
.align 4
.long 0x1BADB002
.long 0
.long -(0x1BADB002)

.section .text
.global start
start:
	cli
	pushl %ebx
	pushl %eax
	call kernel_main

hang:
	hlt
	jmp hang
