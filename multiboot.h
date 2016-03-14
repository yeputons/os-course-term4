#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

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

#endif
