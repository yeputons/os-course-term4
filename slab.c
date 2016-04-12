#include <stdbool.h>
#include "slab.h"
#include "memory.h"
#include "util.h"
#include "printf.h"

/**
 * Single slab cannot exist without slab_allocator, as the latter holds
 * everything about inner structure of slab: amount of nodes (available memory), layout, etc.
 * Slab layout: 'struct slab' comes first, then some bytes are skipped for proper alignment, then actual data comes one after another,
 * so it looks very alike slab for "big" objects
 */

struct slab_node {
    int next_free;
};
struct slab {
    struct slab *prev_slab, *next_slab;
    int first_free;
    int allocated;
    struct slab_node nodes[];
};

#define SLAB_DEBUG

// ========== INDIVIDUAL SLABS ==========
#define MIN_SLAB_NODES 10
#define align_by(x, align) (((x) + (align) - 1) / (align) * (align))

// Given size of item and required alignment calculates parameters of a single slab
static void calculate_slab(size_t item_size, size_t align, size_t *out_slab_nodes_start, int *out_nodes, size_t *out_alloc_mem) {
    #ifdef SLAB_DEBUG
    printf("get_slab_size(item_size=%zd, align=%zd)\n", item_size, align);
    #endif
    assert(item_size > 0 && align >= 1);
    assert((align & (align - 1)) == 0);
    assert(item_size % align == 0);

    size_t per_node = item_size + sizeof(struct slab_node);
    
    size_t alloc_mem = PAGE_SIZE;
    {
        if (alloc_mem < align) {
            alloc_mem = align;
        }
        size_t min_mem = sizeof(struct slab) + per_node * MIN_SLAB_NODES;
        while (alloc_mem < min_mem) {
            alloc_mem *= 2;
        }
    }
    
    // Proof that the following formula is correct despite alignment of the header:
    // whole SLAB is aligned by 'align', so we can to assume that nodes are
    // placed backwards: from its end to beginning until all nodes are placed.
    // Afterwards we will have enough memory to fit header plus node information.
    int nodes = (alloc_mem - sizeof(struct slab)) / per_node;
    size_t header = sizeof(struct slab) + nodes * sizeof(struct slab_node);
    size_t nodes_start = align_by(header, align);

    #ifdef SLAB_DEBUG
    size_t utilized = header + per_node * nodes;
    printf("  Allocate space for %d nodes per slab, header size is %d\n"
            "  first node is at %d, utilized memory is %d/%d\n", nodes, header, nodes_start, utilized, alloc_mem);
    #endif
    assert(nodes_start + nodes * item_size <= alloc_mem);
    
    *out_slab_nodes_start = nodes_start;
    *out_nodes = nodes;
    *out_alloc_mem = alloc_mem;
}

static struct slab* slab_create(struct slab_allocator *a) {
    struct slab* slab = va(alloc_big_phys_aligned(a->slab_alloc_mem));
    slab->prev_slab = NULL;
    slab->next_slab = NULL;
    slab->first_free = 0;
    slab->allocated = 0;
    for (int i = 0; i < a->slab_nodes; i++) {
        slab->nodes[i].next_free = (i + 1 < a->slab_nodes ? i + 1 : -1);
    }
    return slab;
}

static void* slab_alloc(struct slab_allocator *a, struct slab* slab) {
    int id = slab->first_free;
    assert(id >= 0);
    slab->allocated++;
    slab->first_free = slab->nodes[id].next_free;
    slab->nodes[id].next_free = id;
    return ((char*)slab + a->slab_nodes_start + a->item_size * id);
}

static void slab_free(struct slab_allocator *a, struct slab* slab, void *ptr) {
    int id = ((char*)ptr - (char*)slab - a->slab_nodes_start) / a->item_size;
    assert(slab->nodes[id].next_free == id);
    assert(slab->allocated >= 1);
    slab->nodes[id].next_free = slab->first_free;
    slab->first_free = id;
    slab->allocated--;
}

static void slab_destroy(struct slab* slab) {
    assert(!slab->allocated);
    assert(!slab->prev_slab);
    assert(!slab->next_slab);
    free_big_phys_aligned(pa(slab));
}

static void slab_debug_print(struct slab_allocator *a, struct slab *s) {
    printf("slab@%p; prev=%p, next=%p\n", s, s->prev_slab, s->next_slab);
    printf("  allocated=%d, first_free=%d\n", s->allocated, s->first_free);
    for (int i = 0; i < a->slab_nodes; i++) {
        printf("  %3d: ", i);
        if (s->nodes[i].next_free == i) {
            printf("allocated\n");
        } else {
            printf("next_free=%d\n", s->nodes[i]);
        }
    }
}

// ========== SLAB ALLOCATOR ==========

void slab_allocator_init(struct slab_allocator* a, size_t item_size, size_t align) {
    a->item_size = align_by(item_size, align);
    calculate_slab(a->item_size, align, &a->slab_nodes_start, &a->slab_nodes, &a->slab_alloc_mem);
    a->partials = NULL;
    a->fulls_cnt = 0;
}

void* slab_allocator_alloc(struct slab_allocator *a) {
    #ifdef SLAB_DEBUG
    printf("slab_allocator_alloc()\n");
    #endif
    if (!a->partials) {
        a->partials = slab_create(a);
        #ifdef SLAB_DEBUG
        printf("  Create new empty slab@%p\n", a->partials);
        #endif
    }
    struct slab* s = a->partials;
    void* result = slab_alloc(a, s);
    #ifdef SLAB_DEBUG
    printf("  Slab@%p allocated %p\n", s, result);
    #endif
    if (s->allocated == a->slab_nodes) {
        #ifdef SLAB_DEBUG
        printf("  Move slab@%p from partials to fulls\n", s);
        #endif
        // move from partials to fulls
        a->partials = s->next_slab;
        if (a->partials) {
            assert(a->partials->prev_slab == s);
            a->partials->prev_slab = NULL;
        }
        
        s->prev_slab = NULL;
        s->next_slab = NULL;
        a->fulls_cnt++;
    }
    return result;
}

void slab_allocator_free(struct slab_allocator *a, void *ptr) {
    #ifdef SLAB_DEBUG
    printf("slab_allocator_free()\n");
    #endif
    struct slab* s = va(get_big_phys_aligned_block_start(pa(ptr)));
    #ifdef SLAB_DEBUG
    printf("  Pointer %p lies in slab@%p\n", ptr, s);
    #endif
    bool was_full = s->allocated == a->slab_nodes;
    slab_free(a, s, ptr);
    if (was_full) {
        #ifdef SLAB_DEBUG
        printf("  Adding slab back to partials\n");
        #endif
        // move from fulls to partials
        a->fulls_cnt--;
        s->prev_slab = NULL;
        s->next_slab = a->partials;
        if (s->next_slab) {
            assert(s->next_slab->prev_slab == NULL);
            s->next_slab->prev_slab = s;
        }
        a->partials = s;
    }
    if (s->allocated == 0) {
        #ifdef SLAB_DEBUG
        printf("  Slab become empty, removing\n");
        #endif
        // remove
        struct slab* p = s->prev_slab, *n = s->next_slab;
        if (p) {
            p->next_slab = n;
        } else {
            assert(a->partials == s);
            a->partials = n;
        }
        if (n) {
            n->prev_slab = p;
        }
        s->prev_slab = s->next_slab = NULL;
        slab_destroy(s);
    }
}

void slab_allocator_debug_print(struct slab_allocator *a) {
    printf("slab_allocator@%p for items of size %d (aligned)\n", a, a->item_size);
    printf("  slab config: nodes start at %d, bytes/slab = %d, nodes/slab = %d\n", a->slab_nodes_start, a->slab_alloc_mem, a->slab_nodes);
    printf("  %d slabs are fully allocated, partially allocated follow:\n", a->fulls_cnt);
    for (struct slab *s = a->partials; s; s = s->next_slab) {
        slab_debug_print(a, s);
    }
    printf("=====\n");
}


void slab_allocator_deinit(struct slab_allocator *a) {
    assert(!a->fulls_cnt);
    assert(!a->partials);
}
