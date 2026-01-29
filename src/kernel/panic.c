#include "drivers/terminal.h"

void panic(const char *msg, const int width) {
    for (int i = 0; i < width; i++)
        term_puts("-");
    term_puts("[ PANIC: ");
    term_puts(msg);
    term_puts(" ]");
    for (int i = 0; i < width; i++)
        term_puts("-");
    
    __asm__ volatile (
        "cli\n\t"
        "1: hlt\n\t"
        "jmp 1b"
    );
}
