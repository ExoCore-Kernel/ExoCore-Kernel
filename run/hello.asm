BITS 64
GLOBAL _start
_start:
    mov rsi, msg
.loop:
    lodsb
    test al, al
    jz .done
    mov dx, 0xE9
    out dx, al
    jmp .loop
.done:
    ret
msg:
    db 'hello from c.execo', 10, 0
