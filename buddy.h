#ifndef _BUDDY_H
#define _BUDDY_H

#include <stdint.h>
#include <stddef.h>

#define BUDDY_LEVELS (30 - 12 + 1) // max is 1GB page, min is 4KB page
#define BUDDIES_CNT (1 << (BUDDY_LEVELS + 1))

typedef uint64_t buddy_ptr_t;
typedef uint64_t buddy_size_t;

struct buddy_info {
    int prev_free;
    int next_free;
    unsigned is_free:1;
    unsigned allocated_level:5;
};

struct buddy_allocator {
    buddy_ptr_t start;
    struct buddy_info buddies[BUDDIES_CNT];
    int firsts[BUDDY_LEVELS];
};

#define BUDDY_INIT_RESERVE_SEGMENT 1
#define BUDDY_INIT_MAKE_SEGMENT_AVAILABLE 2
struct buddy_init_operation {
    char operation;
    buddy_ptr_t start, end;
};

void buddy_init(struct buddy_allocator *a, buddy_ptr_t start, size_t init_ops_cnt, struct buddy_init_operation *ops);
void buddy_debug_print(struct buddy_allocator *a);

buddy_ptr_t buddy_alloc(struct buddy_allocator *a, buddy_size_t size);
buddy_ptr_t buddy_get_block_start(struct buddy_allocator *a, buddy_ptr_t ptr);
void buddy_free(struct buddy_allocator *a, buddy_ptr_t ptr);

#endif
