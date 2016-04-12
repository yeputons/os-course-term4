#include "memory.h"
#include "printf.h"
#include "util.h"
#include "multiboot.h"
#include "buddy.h"
#include "paging.h"

struct buddy_allocator first_buddy;

extern char text_phys_begin[];
extern char bss_phys_end[];

uint64_t phys_mem_end;

void init_memory(void) {
    if (!(mboot_info->flags & (1 << 6))) {
        die("Memory map unavailable from multiboot\n");
    }

    phys_mem_end = 0;
    uint32_t pos = 0;
    while (pos < mboot_info->mmap_length) {
        struct mboot_memory_segm *segm = (void*)(uint64_t)(mboot_info->mmap_addr + pos);
        printf("Memory segment [%016p..%016p) is %s\n", segm->base_addr, segm->base_addr + segm->length - 1, segm->type == 1 ? "available" : "unavailable");

        uint64_t end = segm->base_addr + segm->length;
        if (end > phys_mem_end) {
            phys_mem_end = end;
        }
        pos += segm->size + 4;
    }
    
    if ((uint64_t)mboot_info + sizeof(struct mboot_info) > 1024 * 1024) {
        die("mboot_info ends in upper memory");
    }
    if (mboot_info->mmap_addr + mboot_info->mmap_length > 1024 * 1024) {
        die("mboot_info->mmap ends in upper memory");
    }
    printf("Kernel resides [%016p..%016p)\n", text_phys_begin, bss_phys_end);

    printf("Initializing buddy for first 1GB... ");
    buddy_init(&first_buddy, 0);
    printf("OK\n");
    buddy_debug_print(&first_buddy);

    printf("Initializing paging... ");
    init_paging();
    printf("OK\n");
}

phys_t alloc_big_phys_aligned(size_t size) {
    return buddy_alloc(&first_buddy, size);
}

void free_big_phys_aligned(phys_t ptr) {
    buddy_free(&first_buddy, ptr);
}
