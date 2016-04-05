#ifndef _SLAB_H
#define _SLAB_H

#include <stddef.h>
#include "threading.h"

struct slab_allocator {
    size_t item_size;
    size_t slab_nodes_start, slab_alloc_mem;
    struct slab *partials;
    int slab_nodes;
    int fulls_cnt;
    spin_lock_t lock;
};

void slab_allocator_init(struct slab_allocator* a, size_t item_size, size_t align);
void* slab_allocator_alloc(struct slab_allocator *a);
void slab_allocator_free(struct slab_allocator *a, void *ptr);
void slab_allocator_deinit(struct slab_allocator *a);

void slab_allocator_debug_print(struct slab_allocator *a);

#endif
