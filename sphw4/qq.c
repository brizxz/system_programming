#include "qq.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static void *action(void *pool_v) {
    tpool *pool = (tpool *)pool_v;

    while (1) {
        pthread_mutex_lock(pool->lock);
        while (pool->task_num == 0 && pool->finished == 0) pthread_cond_wait(pool->cond, pool->lock);
        if (pool->task_num > 0) {
            task *my_task = pool->head;
            pool->head = pool->head->next;
            pool->task_num--;

            pthread_mutex_unlock(pool->lock);

            my_task->func(my_task->arg);

            free(my_task);
        } else if (pool->finished != 0) {
            pthread_mutex_unlock(pool->lock);
            break;
        }
    }
}

tpool *tpool_init(int n_threads) {
    tpool *pool = malloc(sizeof(tpool));
    pool->threads = calloc((size_t)n_threads, sizeof(pthread_t));
    pool->size = n_threads;

    pool->head = NULL;
    pool->tail = NULL;

    pool->task_num = 0;
    pool->finished = 0;

    pool->cond = malloc(sizeof(pthread_cond_t));
    pool->lock = malloc(sizeof(pthread_mutex_t));
    pthread_cond_init(pool->cond, NULL);
    pthread_mutex_init(pool->lock, NULL);

    for (int i = 0; i < n_threads; i++) pthread_create(&pool->threads[i], NULL, action, pool);

    return pool;
}

void tpool_add(tpool *pool, void *(*func)(void *), void *arg) {
    pthread_mutex_lock(pool->lock);

    task *new_task = malloc(sizeof(task));
    new_task->func = func;
    new_task->arg = arg;
    new_task->next = NULL;

    if (pool->head == NULL) {
        pool->head = new_task;
    } else {
        pool->tail->next = new_task;
    }
    pool->tail = new_task;
    pool->task_num++;

    pthread_mutex_unlock(pool->lock);
    pthread_cond_signal(pool->cond);
}

void tpool_wait(tpool *pool) {
    pthread_mutex_lock(pool->lock);

    pool->finished = 1;

    pthread_cond_broadcast(pool->cond);
    pthread_mutex_unlock(pool->lock);

    for (int i = 0; i < pool->size; i++) pthread_join(pool->threads[i], NULL);
}

void tpool_destroy(tpool *pool) {
    free(pool->threads);

    pthread_cond_destroy(pool->cond);
    pthread_mutex_destroy(pool->lock);
    free(pool->cond);
    free(pool->lock);

    free(pool);
}