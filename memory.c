#include "memory.h"
#include "printf.h"
#include "util.h"
#include "multiboot.h"
#include "buddy.h"
#include "paging.h"
#include "slab.h"
#include "threading.h"

extern struct multiboot_info *mboot_info;

struct buddy_allocator first_buddy;

extern char text_phys_begin[];
extern char bss_phys_end[];

phys_t phys_mem_end;

void init_memory(void) {
    if (!(mboot_info->flags & (1 << 6))) {
        die("Memory map unavailable from multiboot\n");
    }

    phys_mem_end = 0;
    uint32_t pos = 0;

    #define MAX_INIT_OPS 64
    static struct buddy_init_operation ops[MAX_INIT_OPS];
    int init_ops = 0;
    while (pos < mboot_info->mmap_length) {
        struct multiboot_mmap_entry *segm = (void*)(uint64_t)(mboot_info->mmap_addr + pos);
        printf("Memory segment [%012llx..%012llx) is %s\n", segm->addr, segm->addr + segm->len, segm->type == MULTIBOOT_MEMORY_AVAILABLE ? "available" : "unavailable");

        if (segm->type == MULTIBOOT_MEMORY_AVAILABLE) {
            assert(init_ops < MAX_INIT_OPS);
            ops[init_ops].operation = BUDDY_INIT_MAKE_SEGMENT_AVAILABLE;
            ops[init_ops].start = segm->addr;
            ops[init_ops].end = segm->addr + segm->len;
            init_ops++;
        }

        phys_t end = segm->addr + segm->len;
        if (end > phys_mem_end) {
            phys_mem_end = end;
        }
        pos += segm->size + 4;
    }
    
    if ((uint64_t)mboot_info + sizeof(struct multiboot_info) > 1024 * 1024) {
        die("mboot_info ends in upper memory");
    }
    if (mboot_info->mmap_addr + mboot_info->mmap_length > 1024 * 1024) {
        die("mboot_info->mmap ends in upper memory");
    }
    printf("Kernel resides [%012llx..%012llx)\n", text_phys_begin, bss_phys_end);

    assert(init_ops < MAX_INIT_OPS);
    ops[init_ops].operation = BUDDY_INIT_RESERVE_SEGMENT;
    ops[init_ops].start = (uint64_t)text_phys_begin;
    ops[init_ops].end = (uint64_t)bss_phys_end;
    init_ops++;
    assert(init_ops < MAX_INIT_OPS);
    ops[init_ops].operation = BUDDY_INIT_RESERVE_SEGMENT;
    ops[init_ops].start = 0;
    ops[init_ops].end = 1024 * 1024;
    init_ops++;

    printf("Initializing buddy for first 1GB... ");
    buddy_init(&first_buddy, 0, init_ops, ops);
    printf("OK\n");
    buddy_debug_print(&first_buddy);
    printf("\n");

    printf("Initializing paging... ");
    init_paging();
    printf("OK\n");

    printf("Testing slab allocator:\n");
    struct slab_allocator a;
    slab_allocator_init(&a, 1024, 1024);
    int k = 10;
    void **ptrs = va(alloc_big_phys_aligned(sizeof(void*) * k));
    for (int i = 0; i < k; i++) {
        ptrs[i] = slab_allocator_alloc(&a);
    }
    slab_allocator_free(&a, ptrs[1]);
    slab_allocator_free(&a, ptrs[4]);
    slab_allocator_debug_print(&a);
    slab_allocator_free(&a, ptrs[0]);
    slab_allocator_free(&a, ptrs[2]);
    slab_allocator_debug_print(&a);
    slab_allocator_free(&a, ptrs[3]);
    for (int i = 5; i < k; i++) {
        slab_allocator_free(&a, ptrs[i]);
    }
    slab_allocator_deinit(&a);
    printf("Done with testing slab\n\n");
}

spin_lock_t phys_alloc_lock;

phys_t alloc_big_phys_aligned(size_t size) {
    spin_lock(&phys_alloc_lock);
    phys_t result = buddy_alloc(&first_buddy, size);
    spin_unlock(&phys_alloc_lock);
    return result;
}

phys_t get_big_phys_aligned_block_start(phys_t ptr) {
    spin_lock(&phys_alloc_lock);
    phys_t result = buddy_get_block_start(&first_buddy, ptr);
    spin_unlock(&phys_alloc_lock);
    return result;
}

void free_big_phys_aligned(phys_t ptr) {
    spin_lock(&phys_alloc_lock);
    buddy_free(&first_buddy, ptr);
    spin_unlock(&phys_alloc_lock);
}

spin_lock_t valloc_lock;

void *valloc(size_t size) {
    spin_lock(&valloc_lock);
    void* result = va(alloc_big_phys_aligned(size));
    spin_unlock(&valloc_lock);
    return result;
}

void vfree(void *ptr) {
    spin_lock(&valloc_lock);
    free_big_phys_aligned(pa(ptr));
    spin_unlock(&valloc_lock);
}
