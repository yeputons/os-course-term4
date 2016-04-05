#ifndef _THREADS_H
#define _THREADS_H

#include <stdbool.h>
#include "list.h"

typedef struct thread *thread_t;

void init_threading(void);

#define THREAD_RUNNING 1
#define THREAD_SLEEPING 2
#define THREAD_TERMINATED 3

thread_t create_thread(void (*entry)(void*), void* arg);
int get_thread_state(thread_t t);
void sleep(void);
void wake(thread_t t);
void wait(thread_t t);
void thread_exit(void);

typedef struct {
    int locked;
    thread_t owner;
} spin_lock_t;
void spin_lock(spin_lock_t *lock);
void spin_unlock(spin_lock_t *lock);

void yield(void);

#endif
