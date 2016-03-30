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

void init_threading(void) {
    slab_allocator_init(&threads_alloc, sizeof(struct thread_t), 4096);
    current_thread = slab_allocator_alloc(&threads_alloc);
    current_thread->next = current_thread;
}

void thread_entry(void);

#define THREAD_STACK_SIZE 8192
thread_t create_thread(void (*entry)(void*), void* arg) {

    uint64_t rflags;
    __asm__("pushfq\npop %0" : "=g"(rflags));
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

    thread_t t = slab_allocator_alloc(&threads_alloc);
    t->rsp = stack;
    t->next = current_thread->next;
    current_thread->next = t;
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
