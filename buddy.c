#include "buddy.h"
#include "printf.h"
#include "util.h"
#include "multiboot.h"

#define MIN_PAGE_SIZE 4096
#define MAX_PAGE_SIZE (1LL << 30)

//#define BUDDY_DEBUG

void add_one(struct buddy_allocator *a, int lev, int i) {
    assert(!a->buddies[i].is_free);
    a->buddies[i].is_free = 1;
    assert(a->buddies[i].prev_free == -1);
    assert(a->buddies[i].next_free == -1);
    int n = a->firsts[lev];
    if (n >= 0) {
        a->buddies[i].next_free = n;
        assert(a->buddies[n].prev_free == -1);
        a->buddies[n].prev_free = i;
    }
    a->firsts[lev] = i;
}

void remove_one(struct buddy_allocator *a, int lev, int i) {
    assert(a->buddies[i].is_free);
    int p = a->buddies[i].prev_free;
    int n = a->buddies[i].next_free;
    a->buddies[i].is_free = 0;
    a->buddies[i].prev_free =
    a->buddies[i].next_free = -1;
    if (p >= 0) {
        assert(a->buddies[p].next_free == i);
        a->buddies[p].next_free = n;
    } else {
        assert(a->firsts[lev] == i);
        a->firsts[lev] = n;
    }
    if (n >= 0) {
        assert(a->buddies[n].prev_free == i);
        a->buddies[n].prev_free = p;
    }
}
    
#define LAST_LEV (BUDDY_LEVELS - 1)
#define LAST_LEV_START (1 << LAST_LEV)
#define LAST_LEV_END (LAST_LEV_START + (1 << LAST_LEV))

#define lies_on_level(lev, i) ((1 << (lev)) <= (i) && (i) <= (2 << (lev)))
#define buddy_size(lev) (MIN_PAGE_SIZE << (LAST_LEV - (lev)))

void reserve_for_init(struct buddy_allocator *a, uint64_t start, uint64_t end) {
    #ifdef BUDDY_DEBUG
    printf("reserve_for_init %p..%p\n", start, end);
    #endif
    if (a->start > start) {
        start = a->start;
    }
    if (a->start + MAX_PAGE_SIZE < end) {
        end = a->start + MAX_PAGE_SIZE;
    }
    if (start >= end) {
        return;
    }
    start = (start - a->start) / MIN_PAGE_SIZE;
    end = (end - a->start + MIN_PAGE_SIZE - 1) / MIN_PAGE_SIZE;
    if (start >= end) {
        return;
    }
    start += LAST_LEV_START;
    end += LAST_LEV_START;
    assert(start < end && end <= BUDDIES_CNT);
    for (int i = start; i < (int)end; i++) {
        if (a->buddies[i].is_free) {
            remove_one(a, LAST_LEV, i);
        }
    }
}

int try_merge_one(struct buddy_allocator *a, int lev, int i) {
    assert(lies_on_level(lev, i));
    assert(lev >= 0);
    if (lev == 0) {
        return 0;
    }
    if (i & 1) {
        i ^= 1;
    }
    if (!a->buddies[i].is_free || !a->buddies[i + 1].is_free) {
        return 0;
    }
    assert(lev > 0);
    remove_one(a, lev, i);
    remove_one(a, lev, i + 1);
    add_one(a, lev - 1, i / 2);
    return 1;
}

void try_merge_rec(struct buddy_allocator *a, int lev, int i) {
    if (try_merge_one(a, lev, i)) {
        try_merge_rec(a, lev - 1, i / 2);
    }
}

void print(struct buddy_allocator *a, int lev, int i) {
    if (lev >= BUDDY_LEVELS) {
        return;
    }
    assert(lies_on_level(lev, i));
    
    if (a->buddies[i].is_free) {
        uint64_t buddy_size = buddy_size(lev);
        uint64_t buddy_id = i - (1 << lev);
        printf("%5d@%2d: [%p..%p); prev=%d, next=%d\n", i, lev, a->start + buddy_id * buddy_size, a->start + (buddy_id + 1) * buddy_size, a->buddies[i].prev_free, a->buddies[i].next_free);
    } else {
        assert(a->buddies[i].prev_free == -1);
        assert(a->buddies[i].next_free == -1);
    }
    print(a, lev + 1, 2 * i);
    print(a, lev + 1, 2 * i + 1);
}

void buddy_debug_print(struct buddy_allocator *a) {
    print(a, 0, 1);
}

extern char text_phys_begin[];
extern char bss_phys_end[];

void buddy_init(struct buddy_allocator *a, uint64_t start) {
    a->start = start;
    assert(a->start < (1LL << 48));
    assert(a->start % MAX_PAGE_SIZE == 0);
    for (int i = 0; i < BUDDIES_CNT; i++) {
        a->buddies[i].prev_free = -1;
        a->buddies[i].next_free = -1;
        a->buddies[i].is_free = 0;
    }
    for (int i = LAST_LEV_START; i < LAST_LEV_END; i++) {
        a->buddies[i].prev_free = (i > LAST_LEV_START) ? i - 1 : -1;
        a->buddies[i].next_free = (i + 1 < LAST_LEV_END) ? i + 1 : -1;
        a->buddies[i].is_free = 1;
    }
    for (int i = 0; i < BUDDY_LEVELS; i++) {
        a->firsts[i] = -1;
    }
    a->firsts[LAST_LEV] = LAST_LEV_START;
    
    reserve_for_init(a, 0, 1024 * 1024);
    reserve_for_init(a, (uint64_t)text_phys_begin, (uint64_t)bss_phys_end);

    struct mboot_memory_segm *segm = (void*)(uint64_t)mboot_info->mmap_addr;
    struct mboot_memory_segm *segm_end = (void*)((uint64_t)mboot_info->mmap_addr + mboot_info->mmap_length);
    while (segm < segm_end) {
        if (segm->type != 1) {
            reserve_for_init(a, segm->base_addr, segm->base_addr + segm->length);
        }
        segm = (void*)((uint64_t)segm + segm->size + 4);
    }
    
    for (int lev = LAST_LEV; lev >= 0; lev--) {
        int start = (1 << lev), end = start + (1 << lev);
        for (int i = start; i < end; i += 2) {
            try_merge_one(a, lev, i);
        }
    }
}


int get_from_level(struct buddy_allocator *a, int lev) {
    #ifdef BUDDY_DEBUG
    printf("  get_from_level(%d)\n", lev);
    #endif
    if (a->firsts[lev] >= 0) {
        int result = a->firsts[lev];
        remove_one(a, lev, result);
        #ifdef BUDDY_DEBUG
        printf("  returning %d\n", result);
        #endif
        return result;
    }
    if (lev == 0) {
        die("Buddy allocator is unable to find memory");
    }
    #ifdef BUDDY_DEBUG
    printf("  going to parent\n");
    #endif
    int parent = get_from_level(a, lev - 1);
    #ifdef BUDDY_DEBUG
    printf("  received %d, putting back %d, returning %d\n", parent, 2 * parent + 1, 2 * parent);
    #endif
    add_one(a, lev, 2 * parent + 1);
    return 2 * parent;
}

uint64_t buddy_alloc(struct buddy_allocator *a, uint64_t size) {
    assert(1 <= size && size <= MAX_PAGE_SIZE);
    int lev = LAST_LEV;
    while (buddy_size(lev) < (int)size) {
        lev--;
        assert(lev >= 0);
    }
    #ifdef BUDDY_DEBUG
    printf("buddy_alloc: size=%lld, lev=%d\n", size, lev);
    #endif
    int result = get_from_level(a, lev);
    assert(lies_on_level(lev, result));
    result -= 1 << lev;
    return a->start + (uint64_t)result * buddy_size(lev);
}

void buddy_free(struct buddy_allocator *a, uint64_t ptr, uint64_t size) {
    assert(1 <= size && size <= MAX_PAGE_SIZE);
    assert(a->start <= ptr && ptr < a->start + MAX_PAGE_SIZE);
    int lev = LAST_LEV;
    while (buddy_size(lev) < (int)size) {
        lev--;
        assert(lev >= 0);
    }
    #ifdef BUDDY_DEBUG
    printf("buddy_free: %p, size=%llf\n", ptr, size);
    #endif
    ptr -= a->start;
    assert(ptr % buddy_size(lev) == 0);
    int i = (1 << lev) + (ptr / buddy_size(lev));
    add_one(a, lev, i);
    try_merge_rec(a, lev, i);
}