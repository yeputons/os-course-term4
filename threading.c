#include "threading.h"
#include "slab.h"
#include "printf.h"
#include "util.h"
#include "memory.h"

struct thread_t {
    void *rsp;
    thread_t next;
};

struct slab_allocator threads_alloc;

thread_t current_thread;

spin_lock_t serial_lock;
spin_lock_t threads_mgmt_lock;

void init_threading(void) {
    slab_allocator_init(&threads_alloc, sizeof(struct thread_t), 4096);
    spin_lock_init(&serial_lock);
    spin_lock_init(&threads_mgmt_lock);
    current_thread = slab_allocator_alloc(&threads_alloc);
    current_thread->next = current_thread;
}

void thread_entry(void);

#define THREAD_STACK_SIZE 8192
thread_t create_thread(void (*entry)(void*), void* arg) {
    uint64_t rflags = get_rflags();
    rflags &= ~0x000008D5; // clear status bits: CF, PF, AF, ZF, SF, OF
    rflags &= ~(1 << 10); // clear control bit to comply with ABI: DF
    rflags |= 1 << 9; // set IF

    uint64_t *stack = (void*)((char*)valloc(THREAD_STACK_SIZE) + THREAD_STACK_SIZE);

    *--stack = (uint64_t)thread_entry; // return address
    *--stack = rflags; // rflags
    *--stack = (uint64_t)arg; // rbx
    *--stack = (uint64_t)entry; // rbp
    *--stack = 0; // r12
    *--stack = 0; // r13
    *--stack = 0; // r14
    *--stack = 0; // r15

    spin_lock(&threads_mgmt_lock);
    thread_t t = slab_allocator_alloc(&threads_alloc);
    t->rsp = stack;
    t->next = current_thread->next;
    current_thread->next = t;
    spin_unlock(&threads_mgmt_lock);
    return t;
}

void* switch_thread_switch(void* old_rsp) {
    current_thread->rsp = old_rsp;
    current_thread = current_thread->next;
    return current_thread->rsp;
}

void exit_unclean(void) {
    die("Thread exited without calling exit\n");
}

void spin_lock_init(spin_lock_t *lock) {
    lock->locked = 0;
    lock->owner = NULL;
}

void spin_lock(spin_lock_t *lock) {
    uint64_t rflags = get_rflags();
    __asm__("cli");
    while (lock->locked && lock->owner != current_thread) {
        yield();
    }
    lock->locked++;
    lock->owner = current_thread;
    set_rflags(rflags);
}

void spin_unlock(spin_lock_t *lock) {
    uint64_t rflags = get_rflags();
    __asm__("cli");
    assert(lock->locked);
    assert(lock->owner == current_thread);
    if (!--lock->locked) {
        lock->owner = NULL;
    }
    set_rflags(rflags);
}
