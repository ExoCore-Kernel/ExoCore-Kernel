void kernel_main() {
    char *video = (char*)0xB8000;
    *video = 'E';
    for (;;) __asm__("hlt");
}
