#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "array.h"
#include "list.h"

#include <pthread.h>
#include <stdbool.h>

#define THREADPOOL_THREADS 4

typedef struct threadpool_t threadpool_t;
typedef void threadpool_func_t (void *arg);

struct threadpool_t
{
  pthread_mutex_t mtx;
  pthread_cond_t cond;
  array_t threads;
  list_t tasks;
  bool stop;
};

int threadpool_init (threadpool_t *pool, int n);
int threadpool_free (threadpool_t *pool);

int threadpool_post (threadpool_t *pool, threadpool_func_t *func, void *arg);
int threadpool_stop (threadpool_t *pool);

#endif
