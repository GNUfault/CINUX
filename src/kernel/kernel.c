#include <stdint.h>
#include "drivers/power.h"
#include "drivers/terminal.h"
#include "kernel/multiboot.h"
#include "cpu/idt.h"
#include "drivers/pic.h"
#include "cpu/gdt.h"
#include "syscall/syscall.h"
#include "user/user.h"
#include "cpu/tss.h"
#include "drivers/ata.h"
#include "drivers/timer.h"
#include "fs/fat32.h"
#include "kernel/task.h"
#include "lib/libc.h"
#include "drivers/rtc.h"
#include "lib/string.h"
#include "drivers/pci.h"
#include "kernel/memory.h"
#include "cpu/paging.h"
#include "kernel/panic.h"

void kernel_main(uint32_t magic, uint32_t mb_addr) { // Please remove "uint32_t magic"
    (void)magic; // And this
    multiboot_info_t *mbi = (multiboot_info_t*)mb_addr;

    term_init_fb(
        (uint32_t)mbi->framebuffer_addr,
                  mbi->framebuffer_width,
                  mbi->framebuffer_height,
                  mbi->framebuffer_pitch,
                  mbi->framebuffer_bpp
    );

    printk(
        "Welcome to\n"
        "  ,----..     ,---,       ,--.'|               ,--,     ,--,     \n"
        " /   /   \\ ,`--.' |   ,--,:  : |         ,--,  |'. \\   / .`|   \n"
        "|   :     :|   :  :,`--.'`|  ' :       ,'_ /|  ; \\ `\\ /' / ;   \n"
        ".   |  ;. /:   |  '|   :  :  | |  .--. |  | :  `. \\  /  / .'    \n"
        ".   ; /--` |   :  |:   |   \\ | :,'_ /| :  . |   \\  \\/  / ./   \n"
        ";   | ;    '   '  ;|   : '  '; ||  ' | |  . .    \\  \\.'  /     \n"
        "|   : |    |   |  |'   ' ;.    ;|  | ' |  | |     \\  ;  ;       \n"
        ".   | '___ '   :  ;|   | | \\   |:  | | :  ' ;    / \\  \\  \\   \n"
        "'   ; : .'||   |  ''   : |  ; .'|  ; ' |  | '   ;  /\\  \\  \\   \n"
        "'   | '/  :'   :  ||   | '`--'  :  | : ;  ; | ./__;  \\  ;  \\   \n"
        "|   :    / ;   |.' '   : |      '  :  `--'   \\|   : / \\  \\  ; \n"
        " \\   \\ .'  '---'   ;   |.'      :  ,      .-./;   |/   \\  ' | \n"
        "  `---`            '---'         `--`----'    `---'     `--`     \n"
        "\n"
    );

    gdt_init();
    tss_init();
    uint32_t mem_size = 0x10000000;
    if (mbi->flags & (1 << 0)) {
        mem_size = mbi->mem_upper * 1024;
    }
    memory_init(mem_size);
    paging_init();
    pic_remap();
    idt_init();
    ata_init();
    ata_identify();
    fat32_init();
    fat32_mount();
    load_timezone();
    timer_init(100);;
    extern void syscall_handler(void);
    idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEE);
    pci_enumerate();
    pci_register_drivers();
    acpi_init();
    task_init();
    __asm__ volatile("sti");
    
    fat32_file_t init;
    if (fat32_open(&init, "INIT.ELF", FAT32_O_RDONLY) == 0) {
        fat32_close(&init);
        task_create(enter_user, "init");
        task_schedule();
        panic("init killed!!", 52);
    } else {
        panic("init not found!", 51);
    }
}
