#include "drivers/terminal.h"

void panic(const char *msg) {
    printk("PANIC: ");
    printk(msg);
    __asm__ volatile (
        "cli\n\t"
        "1: hlt\n\t"
        "jmp 1b"
    );
}
