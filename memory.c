#include "memory.h"
#include "printf.h"
#include "util.h"
#include "multiboot.h"

void init_memory(void) {
    if (!(mboot_info->flags & (1 << 6))) {
        die("Memory map unavailable from multiboot\n");
    }

    uint32_t pos = 0;
    while (pos < mboot_info->mmap_length) {
        struct mboot_memory_segm *segm = (void*)(uint64_t)(mboot_info->mmap_addr + pos);
        printf("Memory segment [%016p..%016p] is %s\n", segm->base_addr, segm->base_addr + segm->length - 1, segm->type == 1 ? "available" : "unavailable");
        pos += segm->size + 4;
    }
    
    if ((uint64_t)mboot_info + sizeof(struct mboot_info) > 1024 * 1024) {
        die("mboot_info ends in upper memory");
    }
    if (mboot_info->mmap_addr + mboot_info->mmap_length > 1024 * 1024) {
        die("mboot_info->mmap ends in upper memory");
    }
}
