#include "threading.h"
#include "slab.h"
#include "printf.h"
#include "util.h"
#include "memory.h"

struct thread_t {
    void *stack_start;
    void *rsp;
    thread_t prev, next;
    volatile int state;
    spin_lock_t wait_lock;
};

struct slab_allocator threads_alloc;

thread_t current_thread;

spin_lock_t threads_mgmt_lock;

void init_threading(void) {
    slab_allocator_init(&threads_alloc, sizeof(struct thread_t), 4096);
    current_thread = slab_allocator_alloc(&threads_alloc);
    current_thread->prev = current_thread;
    current_thread->next = current_thread;
    current_thread->state = THREAD_RUNNING;
}

void thread_entry(void);

#define THREAD_STACK_SIZE 8192
thread_t create_thread(void (*entry)(void*), void* arg) {
    uint64_t rflags = get_rflags();
    rflags &= ~0x000008D5; // clear status bits: CF, PF, AF, ZF, SF, OF
    rflags &= ~(1 << 10); // clear control bit to comply with ABI: DF
    rflags |= 1 << 9; // set IF

    thread_t t = slab_allocator_alloc(&threads_alloc);
    t->stack_start = valloc(THREAD_STACK_SIZE);
    uint64_t *stack = (void*)((char*)t->stack_start + THREAD_STACK_SIZE);

    *--stack = (uint64_t)thread_entry; // return address
    *--stack = rflags; // rflags
    *--stack = (uint64_t)arg; // rbx
    *--stack = (uint64_t)entry; // rbp
    *--stack = 0; // r12
    *--stack = 0; // r13
    *--stack = 0; // r14
    *--stack = 0; // r15

    spin_lock(&threads_mgmt_lock);
    t->rsp = stack;
    t->prev = current_thread;
    t->next = current_thread->next;
    t->prev->next = t;
    t->next->prev = t;
    t->state = THREAD_RUNNING;
    spin_unlock(&threads_mgmt_lock);
    return t;
}

void* switch_thread_switch(void* old_rsp) {
    current_thread->rsp = old_rsp;
    current_thread = current_thread->next;
    return current_thread->rsp;
}

int get_thread_state(thread_t t) {
    return t->state;
}

void thread_exit() {
    current_thread->state = THREAD_TERMINATED;
    current_thread->prev->next = current_thread->next;
    current_thread->next->prev = current_thread->prev;
    yield();
    die("Returned from yield() in thread_exit()");
}

void wait(thread_t t) {
    while (t->state != THREAD_TERMINATED) {
        yield();
    }
    vfree(t->stack_start);
}

void exit_unclean(void) {
    die("Thread exited without calling exit\n");
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
