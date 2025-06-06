BITS 64
org 0

start:
    mov rdi, message
    mov rsi, 0xb8000
.loop:
    mov al, [rdi]
    test al, al
    jz halt
    mov [rsi], al
    mov byte [rsi+1], 0x0f
    add rdi, 1
    add rsi, 2
    jmp .loop
halt:
    cli
hang:
    hlt
    jmp hang

message: db 'RAW64 OK',0
