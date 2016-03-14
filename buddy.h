#ifndef _BUDDY_H
#define _BUDDY_H

#include <stdint.h>

#define BUDDY_LEVELS (30 - 12 + 1) // max is 1GB page, min is 4KB page
#define BUDDIES_CNT (1 << BUDDY_LEVELS)

struct buddy_info {
    int prev_free:20;
    int next_free:20;
    unsigned is_free:1;
};

struct buddy_allocator {
    uint64_t start;
    struct buddy_info buddies[BUDDIES_CNT];
    int firsts[BUDDY_LEVELS];
};

void buddy_init(struct buddy_allocator *a, uint64_t start);
void buddy_debug_print(struct buddy_allocator *a);

uint64_t buddy_alloc(struct buddy_allocator *a, uint64_t size);
void buddy_free(struct buddy_allocator *a, uint64_t ptr, uint64_t size);

#endif
