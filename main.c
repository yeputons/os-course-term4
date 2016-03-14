#include "serial.h"
#include "interrupt.h"
#include "printf.h"
#include "pic.h"
#include "util.h"

int timer_step;

void timer(struct interrupt_info *info) {
    send_eoi(info->interrupt_id);
    printf("Timer! step=%d\n", timer_step++);
}

struct mboot_info {
    uint32_t flags;
    uint32_t mem_lower, mem_upper;
    uint32_t boot_device, cmdline;
    uint32_t mods_count, mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length, mmap_addr;
} __attribute__((packed));
extern struct mboot_info *mboot_info;

struct mboot_memory_segm {
    uint32_t size;
    uint64_t base_addr, length;
    uint32_t type;
} __attribute__((packed));

void print_memory_map(void) {
    if (!(mboot_info->flags & (1 << 6))) {
        die("Memory map unavailable from multiboot\n");
    }

    uint32_t pos = 0;
    while (pos < mboot_info->mmap_length) {
        struct mboot_memory_segm *segm = (void*)(uint64_t)(mboot_info->mmap_addr + pos);
        printf("Memory segment [%p..%p] is %s\n", segm->base_addr, segm->base_addr + segm->length - 1, segm->type == 1 ? "available" : "unavailable");
        pos += segm->size + 4;
    }
}

void main(void) {
    __asm__("cli");
    init_serial();
    puts("Initialized serial");

    printf("Loading IDT... ");
    init_idt();
    printf("OK\n");

    printf("Starting PIC... ");
    init_pic();
    printf("OK\n");

    printf("Enabling interruptions... ");
    __asm__("sti");
    printf("OK\n");

    print_memory_map();
    for(;;);
}
