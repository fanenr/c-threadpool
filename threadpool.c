#include "threadpool.h"

#include <stdlib.h>
#include <string.h>

typedef struct threadpool_task_t threadpool_task_t;

struct threadpool_task_t
{
  threadpool_func_t *func;
  list_node_t node;
  void *arg;
};

static void *work (void *arg);

void
threadpool_free (threadpool_t *pool)
{
  threadpool_wait (pool);

  size_t size = pool->size;
  pthread_t *threads = pool->threads;

  for (size_t i = 0; i < size; i++)
    pthread_join (threads[i], NULL);

  free (pool->threads);
}

void
threadpool_wait (threadpool_t *pool)
{
  for (;;)
    {
      pthread_mutex_lock (&pool->mtx);

      if (!pool->tasks.size)
        {
          pthread_mutex_unlock (&pool->mtx);
          return;
        }

      pthread_mutex_unlock (&pool->mtx);
      pthread_cond_broadcast (&pool->cond);
    }
}

void
threadpool_run (threadpool_t *pool)
{
  pthread_mutex_lock (&pool->mtx);
  pool->status = THREADPOOL_STS_RUN;
  pthread_mutex_unlock (&pool->mtx);
  pthread_cond_broadcast (&pool->cond);
}

void
threadpool_stop (threadpool_t *pool)
{
  pthread_mutex_lock (&pool->mtx);
  pool->status = THREADPOOL_STS_STOP;
  pthread_mutex_unlock (&pool->mtx);
  pthread_cond_broadcast (&pool->cond);
}

void
threadpool_quit (threadpool_t *pool)
{
  pthread_mutex_lock (&pool->mtx);
  pool->status = THREADPOOL_STS_QUIT;
  pthread_mutex_unlock (&pool->mtx);
  pthread_cond_broadcast (&pool->cond);
}

int
threadpool_init (threadpool_t *pool, size_t n)
{
  if (0 != pthread_mutex_init (&pool->mtx, NULL))
    return THREADPOOL_ERR_MTX;

  if (0 != pthread_cond_init (&pool->cond, NULL))
    return THREADPOOL_ERR_CND;

  memset (pool, 0, sizeof (threadpool_t));

  size_t init = 0;
  pthread_t *threads;
  n = n ? n : THREADPOOL_THREADS;

  if (!(threads = malloc (n * sizeof (pthread_t))))
    return THREADPOOL_ERR_MALLOC;

  for (; init < n; init++)
    if (0 != pthread_create (&threads[init], NULL, work, pool))
      goto err;

  pool->size = n;
  pool->threads = threads;

  return 0;

err:
  for (size_t i = 0; i < init; i++)
    pthread_cancel (threads[i]);

  free (threads);

  return THREADPOOL_ERR_CREATE;
}

int
threadpool_post (threadpool_t *pool, threadpool_func_t *f, void *a)
{
  threadpool_task_t *task;

  if (!(task = malloc (sizeof (threadpool_task_t))))
    return THREADPOOL_ERR_MALLOC;

  task->arg = a;
  task->func = f;

  pthread_mutex_lock (&pool->mtx);

  if (pool->status == THREADPOOL_STS_QUIT)
    {
      free (task);
      pthread_mutex_unlock (&pool->mtx);
      return THREADPOOL_ERR_QUITTED;
    }

  list_push_back (&pool->tasks, &task->node);

  pthread_mutex_unlock (&pool->mtx);
  pthread_cond_signal (&pool->cond);

  return 0;
}

static void *
work (void *arg)
{
  threadpool_t *pool = arg;

  for (;;)
    {
      pthread_mutex_lock (&pool->mtx);

      for (; (pool->status == THREADPOOL_STS_RUN && !pool->tasks.size)
             || pool->status == THREADPOOL_STS_STOP;)
        pthread_cond_wait (&pool->cond, &pool->mtx);

      if (pool->status == THREADPOOL_STS_QUIT)
        {
          pthread_mutex_unlock (&pool->mtx);
          return NULL;
        }

      list_node_t *node = pool->tasks.head;
      list_pop_front (&pool->tasks);

      pthread_mutex_unlock (&pool->mtx);

      threadpool_task_t *task = container_of (node, threadpool_task_t, node);
      task->func (task->arg);

      free (task);
    }

  return NULL;
}
