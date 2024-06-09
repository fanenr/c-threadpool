#include "threadpool.h"

#include <stdlib.h>

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

typedef struct threadpool_task_t threadpool_task_t;

struct threadpool_task_t
{
  threadpool_func_t *func;
  list_node_t node;
  void *arg;
};

static void *work (void *arg);

int
threadpool_init (threadpool_t *pool, int n)
{
  if (0 != pthread_mutex_init (&pool->mtx, NULL))
    return THREADPOOL_MUTEX_INIT;

  if (0 != pthread_cond_init (&pool->cond, NULL))
    return THREADPOOL_COND_INIT;

  pool->stop = false;
  pool->tasks = LIST_INIT;
  pool->threads = ARRAY_INIT;
  pool->threads.element = sizeof (pthread_t);

  if (n <= 0)
    n = THREADPOOL_THREADS;

  int init = 0;
  pthread_t *threads;

  if (!(threads = malloc (n * sizeof (pthread_t))))
    return THREADPOOL_MALLOC;

  for (; init < n; init++)
    if (0 != pthread_create (&threads[init], NULL, work, pool))
      goto err;

  pool->threads.data = threads;
  pool->threads.size = pool->threads.cap = n;

  return 0;

err:
  for (int i = 0; i < init; i++)
    pthread_cancel (threads[i]);

  free (threads);

  return THREADPOOL_THREAD_NEW;
}

int
threadpool_free (threadpool_t *pool)
{
  threadpool_stop (pool);

  size_t nums = pool->threads.size;
  pthread_t *threads = pool->threads.data;

  for (size_t i = 0; i < nums; i++)
    pthread_join (threads[i], NULL);

  free (pool->threads.data);

  return 0;
}

int
threadpool_post (threadpool_t *pool, threadpool_func_t *func, void *arg)
{
  threadpool_task_t *task;

  if (!(task = malloc (sizeof (threadpool_task_t))))
    return THREADPOOL_MALLOC;

  task->arg = arg;
  task->func = func;

  pthread_mutex_lock (&pool->mtx);
  list_push_back (&pool->tasks, &task->node);
  pthread_mutex_unlock (&pool->mtx);

  pthread_cond_signal (&pool->cond);

  return 0;
}

int
threadpool_stop (threadpool_t *pool)
{
  pthread_mutex_lock (&pool->mtx);
  pool->stop = true;
  pthread_mutex_unlock (&pool->mtx);

  return 0;
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
