BITS 32
org 0

; Write ‘1’ at column 2 (row 0) of VGA text mode
mov ebx, 0xB8000
mov byte [ebx+2], '1'
ret
