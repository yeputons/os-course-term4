#ifndef _THREADS_H
#define _THREADS_H

typedef struct thread_t *thread_t;

void init_threading(void);

thread_t create_thread(void (*entry)(void*), void* arg);

void yield(void);

#endif
