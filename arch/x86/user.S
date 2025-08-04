.intel_syntax noprefix
.global enter_user_mode
enter_user_mode:
    mov rcx, rsi        # user stack top
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push 0x23
    push rcx
    pushfq
    pop rax
    or rax, 0x200
    push rax
    push 0x1B
    push rdi
    iretq

.section .note.GNU-stack,"",@progbits
