#include <stdbool.h>
#include "buddy.h"
#include "printf.h"
#include "util.h"

#define MIN_PAGE_SIZE ((buddy_size_t)4096)
#define MAX_PAGE_SIZE (MIN_PAGE_SIZE << (BUDDY_LEVELS - 1))

//#define BUDDY_DEBUG

#ifdef BUDDY_DEBUG
#define dbg(...) printf(__VA_ARGS__)
#else
#define dbg(...)
#endif

// Marks single element as free and adds it to linked list
static void add_one(struct buddy_allocator *a, int lev, int i) {
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

// Marks single element as non-free and removes it from linked list
static void remove_one(struct buddy_allocator *a, int lev, int i) {
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

// Used in initialization for marking consecutive segments on the last level as available/unavailable
// Should not be run after initialization as it works with the last level only
static void mark_for_init(struct buddy_allocator *a, buddy_ptr_t start, buddy_ptr_t end, bool available) {
    dbg("%sreserve_for_init %012llx..%012llx\n", available ? "un" : "", start, end);
    if (a->start > start) {
        start = a->start;
    }
    if (a->start + MAX_PAGE_SIZE < end) {
        end = a->start + MAX_PAGE_SIZE;
    }
    if (start >= end) {
        return;
    }
    if (!available) {
        // extending segment until aligned
        start = (start - a->start) / MIN_PAGE_SIZE;
        end = (end - a->start + MIN_PAGE_SIZE - 1) / MIN_PAGE_SIZE;
    } else {
        // shrinking segment until aligned
        start = (start - a->start + MIN_PAGE_SIZE - 1) / MIN_PAGE_SIZE;
        end = (end - a->start) / MIN_PAGE_SIZE;
    }
    if (start >= end) {
        return;
    }
    start += LAST_LEV_START;
    end += LAST_LEV_START;
    assert(start < end && end <= BUDDIES_CNT);
    for (int i = start; i < (int)end; i++) {
        if (a->buddies[i].is_free && !available) {
            remove_one(a, LAST_LEV, i);
        }
        if (!a->buddies[i].is_free && available) {
            add_one(a, LAST_LEV, i);
        }
    }
}

static bool try_merge_one(struct buddy_allocator *a, int lev, int i) {
    assert(lies_on_level(lev, i));
    assert(lev >= 0);
    if (lev == 0) {
        return false;
    }
    if (i & 1) {
        i ^= 1;
    }
    if (!a->buddies[i].is_free || !a->buddies[i + 1].is_free) {
        return false;
    }
    assert(lev > 0);
    remove_one(a, lev, i);
    remove_one(a, lev, i + 1);
    add_one(a, lev - 1, i / 2);
    return true;
}

static void try_merge_rec(struct buddy_allocator *a, int lev, int i) {
    if (try_merge_one(a, lev, i)) {
        try_merge_rec(a, lev - 1, i / 2);
    }
}

static void print(struct buddy_allocator *a, int lev, int i) {
    if (lev >= BUDDY_LEVELS) {
        return;
    }
    assert(lies_on_level(lev, i));
    
    if (a->buddies[i].is_free) {
        buddy_size_t buddy_size = buddy_size(lev);
        int buddy_id = i - (1 << lev);
        printf("%5d@%2d: [%012llx..%012llx); prev=%d, next=%d\n", i, lev, a->start + buddy_id * buddy_size, a->start + (buddy_id + 1) * buddy_size, a->buddies[i].prev_free, a->buddies[i].next_free);
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

void buddy_init(struct buddy_allocator *a, buddy_ptr_t start, size_t init_ops_cnt, struct buddy_init_operation *ops) {
    a->start = start;
    assert(a->start < (1LL << 48));
    assert(a->start % MAX_PAGE_SIZE == 0);
    // Initially everything is unavailable
    for (int i = 0; i < BUDDIES_CNT; i++) {
        a->buddies[i].prev_free = -1;
        a->buddies[i].next_free = -1;
        a->buddies[i].is_free = 0;
        a->buddies[i].allocated_level = BUDDY_LEVELS;
    }
    for (int i = 0; i < BUDDY_LEVELS; i++) {
        a->firsts[i] = -1;
    }

    for (size_t i = 0; i < init_ops_cnt; i++) {
        mark_for_init(a, ops[i].start, ops[i].end, ops[i].operation == BUDDY_INIT_MAKE_SEGMENT_AVAILABLE);
    }

    // Initialization is completed, now we can merge available segments into bigger
    for (int lev = LAST_LEV; lev >= 0; lev--) {
        int start = (1 << lev), end = start + (1 << lev);
        for (int i = start; i < end; i += 2) {
            try_merge_one(a, lev, i);
        }
    }
}


// Tries to retrieve a block from level 'lev', splitting bigger blocks if needed
static int get_from_level(struct buddy_allocator *a, int lev) {
    dbg("  get_from_level(%d)\n", lev);
    if (a->firsts[lev] >= 0) {
        int result = a->firsts[lev];
        remove_one(a, lev, result);
        dbg("  returning %d\n", result);
        return result;
    }
    if (lev == 0) {
        die("Buddy allocator is unable to find memory");
    }
    dbg("  going to parent\n");
    int parent = get_from_level(a, lev - 1);
    dbg("  received %d, putting back %d, returning %d\n", parent, 2 * parent + 1, 2 * parent);
    add_one(a, lev, 2 * parent + 1);
    return 2 * parent;
}

buddy_ptr_t buddy_alloc(struct buddy_allocator *a, buddy_size_t size) {
    assert(1 <= size && size <= MAX_PAGE_SIZE);
    int lev = LAST_LEV;
    while (buddy_size(lev) < size) {
        lev--;
        assert(lev >= 0);
    }
    dbg("buddy_alloc: size=%lld, lev=%d\n", size, lev);
    int result = get_from_level(a, lev);
    assert(lies_on_level(lev, result));

    int child = result << (LAST_LEV - lev);
    assert(lies_on_level(LAST_LEV, child));
    assert(a->buddies[child].allocated_level == BUDDY_LEVELS);
    a->buddies[child].allocated_level = lev;
    dbg("  marked child %d with lev=%d\n", child, lev);

    result -= 1 << lev;
    return a->start + result * buddy_size(lev);
}

// Returns start of block which contains 'ptr'
buddy_ptr_t buddy_get_block_start(struct buddy_allocator *a, buddy_ptr_t ptr) {
    assert(a->start <= ptr && ptr < a->start + MAX_PAGE_SIZE);

    int i = (ptr - a->start) / MIN_PAGE_SIZE + LAST_LEV_START;
    int lev = LAST_LEV;

    for (;;) {
        assert(lies_on_level(lev, i));
        int child = i << (LAST_LEV - lev);
        if (a->buddies[child].allocated_level != BUDDY_LEVELS) {
            assert(a->buddies[child].allocated_level <= lev);
            child -= 1 << LAST_LEV;
            return a->start + child * MIN_PAGE_SIZE;
        }
        i /= 2;
        lev--;
        assert(i >= 1);
    }
}

void buddy_free(struct buddy_allocator *a, buddy_ptr_t ptr) {
    assert(a->start <= ptr && ptr < a->start + MAX_PAGE_SIZE);
    assert((ptr - a->start) % MIN_PAGE_SIZE == 0);

    int child = (ptr - a->start) / MIN_PAGE_SIZE + LAST_LEV_START;
    dbg("buddy_free: ptr=%012llx; child=%d\n", ptr, child);
    assert(lies_on_level(LAST_LEV, child));
    int lev = a->buddies[child].allocated_level;
    assert(lev < BUDDY_LEVELS);
    a->buddies[child].allocated_level = BUDDY_LEVELS;
    dbg("  lev=%d, size=%d\n", lev, MIN_PAGE_SIZE << lev);

    ptr -= a->start;
    assert(ptr % buddy_size(lev) == 0);
    int i = (1 << lev) + (ptr / buddy_size(lev));
    add_one(a, lev, i);
    try_merge_rec(a, lev, i);
}
