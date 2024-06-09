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

int
threadpool_init (threadpool_t *pool, size_t n)
{
  if (0 != pthread_mutex_init (&pool->mtx, NULL))
    return THREADPOOL_MUTEX_INIT;

  if (0 != pthread_cond_init (&pool->cond, NULL))
    return THREADPOOL_COND_INIT;

  memset (pool, 0, sizeof (threadpool_t));

  size_t init = 0;
  pthread_t *threads;
  n = n ? n : THREADPOOL_THREADS;

  if (!(threads = malloc (n * sizeof (pthread_t))))
    return THREADPOOL_MALLOC;

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

  return THREADPOOL_THREAD_NEW;
}

void
threadpool_free (threadpool_t *pool)
{
  threadpool_stop (pool);

  size_t size = pool->size;
  pthread_t *threads = pool->threads;

  for (size_t i = 0; i < size; i++)
    pthread_join (threads[i], NULL);

  free (pool->threads);
}

int
threadpool_post (threadpool_t *pool, threadpool_func_t *f, void *a)
{
  threadpool_task_t *task;

  if (!(task = malloc (sizeof (threadpool_task_t))))
    return THREADPOOL_MALLOC;

  task->arg = a;
  task->func = f;

  pthread_mutex_lock (&pool->mtx);
  list_push_back (&pool->tasks, &task->node);
  pthread_mutex_unlock (&pool->mtx);

  pthread_cond_signal (&pool->cond);

  return 0;
}

void
threadpool_stop (threadpool_t *pool)
{
  pthread_mutex_lock (&pool->mtx);
  pool->stop = true;
  pthread_mutex_unlock (&pool->mtx);
}

static void *
work (void *arg)
{
  threadpool_t *pool = arg;

  for (;;)
    {
      pthread_mutex_lock (&pool->mtx);

      for (; !pool->tasks.size && !pool->stop;)
        pthread_cond_wait (&pool->cond, &pool->mtx);

      if (!pool->tasks.size && pool->stop)
        {
          pthread_mutex_unlock (&pool->mtx);
          break;
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
