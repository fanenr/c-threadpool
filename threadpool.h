#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "list.h"

#include <pthread.h>
#include <stdbool.h>

#define THREADPOOL_THREADS 4

enum
{
  THREADPOOL_OK,

  THREADPOOL_MUTEX_INIT,
  THREADPOOL_MUTEX_LOCK,
  THREADPOOL_MUTEX_UNLOCK,

  THREADPOOL_COND_INIT,
  THREADPOOL_COND_WAIT,
  THREADPOOL_COND_SIGNAL,

  THREADPOOL_MALLOC,
  THREADPOOL_THREAD_NEW,
};

typedef struct threadpool_t threadpool_t;
typedef void threadpool_func_t (void *arg);

struct threadpool_t
{
  pthread_mutex_t mtx;
  pthread_cond_t cond;
  pthread_t *threads;
  list_t tasks;
  size_t size;
  bool stop;
};

int threadpool_init (threadpool_t *pool, size_t n);
void threadpool_free (threadpool_t *pool);

int threadpool_post (threadpool_t *pool, threadpool_func_t *f, void *a);
void threadpool_stop (threadpool_t *pool);

#endif
