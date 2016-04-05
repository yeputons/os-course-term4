#ifndef _THREADS_H
#define _THREADS_H

#include <stdbool.h>

typedef struct thread_t *thread_t;

void init_threading(void);

thread_t create_thread(void (*entry)(void*), void* arg);

typedef struct {
    int locked;
    thread_t owner;
} spin_lock_t;
void spin_lock(spin_lock_t *lock);
void spin_unlock(spin_lock_t *lock);

extern spin_lock_t serial_lock;

void yield(void);

#endif