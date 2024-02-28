#include <pthread.h>
#ifndef __MY_THREAD_POOL_H
#define __MY_THREAD_POOL_H

typedef struct task {
    void *(*func)(void *);
    void *arg;
    struct task *next;
} task;

typedef struct tpool {
    int size;
    pthread_t *threads;
    task *head, *tail;
    int task_num;
    int finished;
    pthread_cond_t *cond;
    pthread_mutex_t *lock;
} tpool;

tpool *tpool_init(int n_threads);
void tpool_add(tpool *, void *(*func)(void *), void *);
void tpool_wait(tpool *);
void tpool_destroy(tpool *);

#endif