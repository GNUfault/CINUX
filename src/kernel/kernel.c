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
#include "drivers/mouse.h"
#include "kernel/memory.h"
#include "cpu/paging.h"

void kernel_main(uint32_t magic, uint32_t mb_addr) {
    (void)magic;
    multiboot_info_t *mbi = (multiboot_info_t*)mb_addr;
    int fb_ok = 0;
    
    if (
        (mbi->flags & (1 << 12)) &&
        mbi->framebuffer_type == 1 &&
        (mbi->framebuffer_bpp == 24 || mbi->framebuffer_bpp == 32) &&
        mbi->framebuffer_addr != 0 &&
        mbi->framebuffer_pitch != 0
    )
    {
        term_init_fb(
            (uint32_t)mbi->framebuffer_addr,
            mbi->framebuffer_width,
            mbi->framebuffer_height,
            mbi->framebuffer_pitch,
            mbi->framebuffer_bpp
        );
        fb_ok = 1;
    }
    
    if (!fb_ok)
        term_init();

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
    ata_error_t err = ata_identify();
    if (err != ATA_OK)
    {
    }
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
    
    task_create(enter_user, "init");
    
    task_schedule();
}
