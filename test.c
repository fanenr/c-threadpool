#include "threadpool.h"

#include <stdio.h>

size_t counter;
static void task (void *arg);

int
main (void)
{
  threadpool_t pool;
  size_t tasks = 10000000;

  if (0 != threadpool_init (&pool, 16))
    return 1;

  for (size_t i = 0; i < tasks; i++)
    if (0 != threadpool_post (&pool, task, (void *)i))
      break;

  threadpool_free (&pool);
  printf ("counter: %lu\n", counter);
}

static void
task (void *arg)
{
  size_t i = (size_t)arg;
  __atomic_fetch_add (&counter, 1, __ATOMIC_RELAXED);
}
