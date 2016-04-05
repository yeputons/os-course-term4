#include "threading.h"
#include "slab.h"
#include "printf.h"
#include "util.h"
#include "memory.h"
#include "list.h"

#define THREADING_DEBUG

#ifdef THREADING_DEBUG
#define log(...) printf(__VA_ARGS__)
#else
#define log(...)
#endif

struct thread {
    void *stack_start;
    void *rsp;
    struct list_head list;
    volatile int state;
};
#define UNL(ptr) LIST_ENTRY(ptr, struct thread, list)

struct slab_allocator threads_alloc;

LIST_HEAD(running_threads);
LIST_HEAD(sleeping_threads);
thread_t current_thread;

spin_lock_t threads_mgmt_lock;

void idle_thread(void* arg) {
    (void)arg;
    while (true) {
        yield();
    }
}

void init_threading(void) {
    slab_allocator_init(&threads_alloc, sizeof(struct thread), 1);
    current_thread = slab_allocator_alloc(&threads_alloc);
    current_thread->stack_start = 0;
    current_thread->state = THREAD_RUNNING;
    create_thread(idle_thread, NULL);
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
    log("created thread %p on stack %p, entry is %p(%llx)\n", t, t->stack_start, entry, arg);
    t->rsp = stack;
    t->state = THREAD_RUNNING;
    list_add_tail(&t->list, &running_threads);
    spin_unlock(&threads_mgmt_lock);
    return t;
}

void* switch_thread_switch(void* old_rsp) {
    current_thread->rsp = old_rsp;
    if (current_thread->state == THREAD_RUNNING) {
        list_add_tail(&current_thread->list, &running_threads);
    }
    if (current_thread->state == THREAD_SLEEPING) {
        list_add_tail(&current_thread->list, &sleeping_threads);
    }
    current_thread = UNL(list_first(&running_threads));
    list_del(&current_thread->list);
    return current_thread->rsp;
}

int get_thread_state(thread_t t) {
    return t->state;
}

void thread_exit(void) {
    spin_lock(&threads_mgmt_lock);
    log("thread %p is exiting\n", current_thread);
    current_thread->state = THREAD_TERMINATED;
    spin_unlock(&threads_mgmt_lock);
    yield();
    die("Returned from yield() in thread_exit()");
}

void sleep(void) {
    spin_lock(&threads_mgmt_lock);
    log("thread %p is sleeping\n", current_thread);
    current_thread->state = THREAD_SLEEPING;
    spin_unlock(&threads_mgmt_lock);
    yield();
}

void wake(thread_t t) {
    spin_lock(&threads_mgmt_lock);
    if (t->state == THREAD_SLEEPING) {
        t->state = THREAD_RUNNING;
        log("waking up thread %p\n", t);
        list_del(&t->list);
        list_add_tail(&t->list, &running_threads);
    }
    spin_unlock(&threads_mgmt_lock);
}

void wait(thread_t t) {
    while (t->state != THREAD_TERMINATED) {
        yield();
    }
    log("terminated thread %p and freed its stack %p\n", t, t->stack_start);
    vfree(t->stack_start);
    slab_allocator_free(&threads_alloc, t);
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
