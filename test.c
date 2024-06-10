#include "threadpool.h"

#include <stdio.h>
#include <unistd.h>

static void task (void *arg);

int
main (void)
{
  threadpool_t pool;
  size_t tasks = 10000;

  if (0 != threadpool_init (&pool, 16))
    return 1;

  for (size_t i = 0; i < tasks; i++)
    if (0 != threadpool_post (&pool, task, (void *)i))
      break;

  threadpool_free (&pool);
}

static void
task (void *arg)
{
  size_t i = (size_t)arg;
  printf ("%lu ", i);
}
