#include "my_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static void *worker(void *pool1){
    tpool *pool = (tpool *)pool1;
    while (1){
        pthread_mutex_lock(pool->work_mutex);
        while (pool->task_num == 0 && pool->finish_flag == 0){
            pthread_cond_wait(pool->work_cond, pool->work_mutex);
        }

        if (pool->task_num > 0) {
            tpool_work *my_task = pool->work_first;
            pool->work_first = pool->work_first->next;
            pool->task_num--;

            pthread_mutex_unlock(pool->work_mutex);
            my_task->func(my_task->arg);
            free(my_task);
        } 
        else if (pool->finish_flag != 0) {
            pthread_mutex_unlock(pool->work_mutex);
            break;
        }
    }

}
void tpool_add(tpool *pool, thread_func_t func, void *arg) {
    // TODO
    pthread_mutex_lock(pool->work_mutex);
    tpool_work *new_work = malloc(sizeof(tpool_work));
    new_work->func = func;
    new_work->arg = arg;
    new_work->next = NULL;

    if (pool->work_first == NULL) pool->work_first = new_work;
    else pool->work_last->next = new_work;
    pool->work_last = new_work;
    pool->task_num++;

    pthread_mutex_unlock(pool->work_mutex);
    pthread_cond_signal(pool->work_cond);
}
void tpool_wait(tpool *pool) {
    // TODO
    pthread_mutex_lock(pool->work_mutex);
    pool->finish_flag = 1;
    pthread_cond_broadcast(pool->work_cond);
    pthread_mutex_unlock(pool->work_mutex);
    // wait thread finish
    for (int i = 0; i < pool->size; i++) pthread_join(pool->threads[i], NULL);
}
void tpool_destroy(tpool *pool) {
    // TODO
    free(pool->threads);
    pthread_cond_destroy(pool->work_cond);
    pthread_mutex_destroy(pool->work_mutex);
    free(pool->work_cond); free(pool->work_mutex);
    free(pool);
}
tpool *tpool_init(int n_threads) {
    // TODO
    tpool *pool =  malloc(sizeof(tpool));
    pool->threads = calloc ((size_t)n_threads, sizeof(pthread_t));
    pool->size = n_threads;

    pool->work_first = pool->work_last = NULL;
    pool->task_num = pool->finish_flag = 0;

    pool->work_cond = malloc(sizeof(pthread_cond_t));
    pool->work_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_cond_init(pool->work_cond, NULL);
    pthread_mutex_init(pool->work_mutex, NULL);

    for (int i = 0; i < n_threads; i++) pthread_create(&pool->threads[i], NULL, worker, pool);
    return pool;
}