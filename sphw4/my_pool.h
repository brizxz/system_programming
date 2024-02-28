#include <pthread.h>
#ifndef __MY_THREAD_POOL_H
#define __MY_THREAD_POOL_H

typedef void*(*thread_func_t)(void *);
typedef struct tpool_work {
    thread_func_t func;
    void *arg;
    struct tpool_work *next;
} tpool_work;
typedef struct tpool {
    // TODO: define your structure
    int size; int task_num; 
    pthread_t  *threads;
    tpool_work *work_first; tpool_work *work_last;
    pthread_mutex_t *work_mutex;
    pthread_cond_t *work_cond;
    int finish_flag;
} tpool;

tpool *tpool_init(int n_threads);
void tpool_add(tpool *tm, thread_func_t func, void *arg);
void tpool_wait(tpool *tm);
void tpool_destroy(tpool *tm);

#endif