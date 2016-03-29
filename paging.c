#include "paging.h"
#include "paging_util.h"
#include "printf.h"
#include "util.h"
#include "memory.h"
#include "buddy.h"

#define ENTRIES_PER_TABLE 512
#define TABLE_SIZE (sizeof(pte_t) * ENTRIES_PER_TABLE)

//#define PAGING_DEBUG

#ifdef PAGING_DEBUG
#define dbg(...) printf(__VA_ARGS__)
#else
#define dbg(...)
#endif

static phys_t alloc_page_table() {
    phys_t table = alloc_big_phys_aligned(TABLE_SIZE);
    assert(table % 4096 == 0);
    memset(va(table), 0, TABLE_SIZE);
    return table;
}

static void map_pages(pte_t *ptes, uint64_t base_offset, uint64_t base_step, uint64_t lin_start, uint64_t lin_end, uint64_t phys_start) {
    dbg("map_pages in directory@%p\n", pa(ptes));
    dbg("  directory maps [%012p; %012p); base_step=%llx\n", base_offset, base_offset + ENTRIES_PER_TABLE * base_step, base_step);
    dbg("  want to map    [%012p; %012p) to physical starting at %p\n", lin_start, lin_end, phys_start);
    assert(lin_start < lin_end);
    assert(base_offset % base_step == 0);
    if (lin_start < base_offset) {
        uint64_t shift = base_offset - lin_start;
        lin_start += shift;
        phys_start += shift;
    }
    if (lin_end > base_offset + ENTRIES_PER_TABLE * base_step) {
        lin_end = base_offset + ENTRIES_PER_TABLE * base_step;
    }
    if (lin_start >= lin_end) {
        return;
    }

    int entry_start = (lin_start - base_offset) / base_step;
    int entry_end = (lin_end - base_offset + base_step - 1) / base_step;
    assert(0 <= entry_start && entry_start < entry_end && entry_end <= ENTRIES_PER_TABLE);
    
    bool can_fully_allocate = base_step == 4096 || base_step == 2 * 1024 * 1024;
    dbg("  update entries [%d; %d)\n", entry_start, entry_end);
    for (int i = entry_start; i < entry_end; i++) {
        uint64_t cur_start = base_offset + i * base_step;
        uint64_t cur_end = base_offset + (i + 1) * base_step;
        assert(!(lin_end <= cur_start || cur_end <= lin_start));
        if (ptes[i]) {
            dbg("  entry %d exists: %x\n", i, ptes[i]);
        }
        if (can_fully_allocate && lin_start <= cur_start && cur_end <= lin_end) {
            assert(!pte_present(ptes[i]));
            // fully allocate page
            uint64_t cur_phys_start = phys_start + cur_start - lin_start;
            assert(cur_phys_start % base_step == 0);
            if (base_step == 4096) {
                ptes[i] = PTE_PRESENT | PTE_WRITE | cur_phys_start;
            } else if (base_step == 2 * 1024 * 1024) {
                ptes[i] = PTE_PRESENT | PTE_WRITE | PTE_LARGE | cur_phys_start;
            } else {
                assert(false);
            }
        } else {
            // go deeper
            if (!pte_present(ptes[i])) {
                ptes[i] = PTE_PRESENT | PTE_WRITE | alloc_page_table();
            }
            map_pages(va(pte_phys(ptes[i])), cur_start, base_step / ENTRIES_PER_TABLE, lin_start, lin_end, phys_start);
        }
    }
}

static void print_table(pte_t *ptes, uint64_t base_offset, uint64_t base_step, int offset) {
    for (int i = 0; i < offset; i++) {
        printf(" ");
    }
    printf("table@%8p for [%012p, %012p) (step is %llx)\n", pa(ptes), base_offset, base_offset + ENTRIES_PER_TABLE * base_step, base_step);
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        if (pte_present(ptes[i])) {
            for (int i = 0; i < offset; i++) {
                printf(" ");
            }
            printf("%d is present: w=%d, u=%d, l=%d, p=%p\n", i, pte_write(ptes[i]), pte_user(ptes[i]), pte_large(ptes[i]), pte_phys(ptes[i]));
            if (base_step > 4096 && !pte_large(ptes[i])) {
                print_table(va(pte_phys(ptes[i])), base_offset + base_step * i, base_step / ENTRIES_PER_TABLE, offset + 8);
            }
        }
    }
}

#define PML4_STEP (1LL << 39)

void print_pml4(phys_t addr) {
    print_table(va(addr), 0, PML4_STEP, 0);
}

void init_paging() {
    pte_t *pml4 = va(alloc_page_table());
    map_pages(pml4, 0, PML4_STEP, linear(HIGH_BASE), linear(HIGH_BASE) + phys_mem_end, 0);
    map_pages(pml4, 0, PML4_STEP, linear(KERNEL_BASE), linear(KERNEL_BASE) + 2LL * 1024 * 1024 * 1024, 0);
    //print_table(pml4, 0, PML4_STEP, 0);
    store_pml4(pa(pml4));
    flush_tlb();
}
