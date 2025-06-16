.intel_syntax noprefix
.section .multiboot
.align 4
.long 0x1BADB002       /* magic */
.long 0                  /* flags */
.long -(0x1BADB002)      /* checksum */


.section .bss
.align 4096
pml4:
.fill 512,8,0
pdpt:
.fill 512,8,0
pd:
.fill 512,8,0

stack:
.skip 8192

.section .data
multiboot_magic:
.quad 0
multiboot_info:
.quad 0

.section .text
.code32
.extern idt_init
.global start
start:
cli
mov [multiboot_magic], eax
mov [multiboot_info], ebx
call setup_paging
lgdt gdt64_ptr
mov eax, cr4
or eax, 0x20         /* enable PAE */
mov cr4, eax
mov ecx, 0xC0000080
rdmsr
or eax, 0x100        /* LME */
wrmsr
mov eax, OFFSET pml4
mov cr3, eax
mov eax, cr0
or eax, 0x80000001   /* PG | PE */
mov cr0, eax
    /* Far jump to 64-bit mode */
    .byte 0xEA
    .long long_mode_start
    .word 0x08

setup_paging:
  /* Identity map the first 1 GiB using 2 MiB pages */
  mov ecx, 0
1:
  mov eax, ecx
  shl eax, 21
  or eax, 0x83           /* present | rw | 2MiB page */
  mov [pd+ecx*8], eax
  mov dword ptr [pd+ecx*8+4], 0
  inc ecx
  cmp ecx, 512
  jne 1b
  mov eax, OFFSET pd
  or eax, 3
  mov dword ptr [pdpt], eax
  mov dword ptr [pdpt+4], 0
  mov eax, OFFSET pdpt
  or eax, 3
  mov dword ptr [pml4], eax
  mov dword ptr [pml4+4], 0
  ret

gdt64:
.quad 0
.quad 0x00AF9A000000FFFF
.quad 0x00AF92000000FFFF
gdt64_end:
gdt64_ptr:
.word gdt64_end - gdt64 - 1
.quad gdt64

.code64
long_mode_start:
mov ax, 0x10
mov ds, ax
mov es, ax
mov ss, ax
mov fs, ax
mov gs, ax
lea rsp, [stack + 8192]
call idt_init
mov rdi, [multiboot_magic]
mov rsi, [multiboot_info]
call kernel_main

hang:
hlt
jmp hang

/* Mark stack usage to avoid executable stack warnings */
.section .note.GNU-stack,"",@progbits
