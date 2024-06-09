#include "list.h"

#define gcc_likely(exp) __builtin_expect (!!(exp), 1)
#define gcc_unlikely(exp) __builtin_expect (!!(exp), 0)

list_node_t *
list_at (const list_t *list, size_t index)
{
  if (gcc_unlikely (index >= list->size))
    return NULL;

  list_node_t *ret = list->head;
  for (; index; index--)
    ret = ret->next;

  return ret;
}

void
list_push_front (list_t *list, list_node_t *node)
{
  node->prev = NULL;
  node->next = list->head;

  if (list->head)
    list->head->prev = node;
  else
    list->tail = node;
  list->head = node;

  list->size++;
}

void
list_push_back (list_t *list, list_node_t *node)
{
  node->next = NULL;
  node->prev = list->tail;

  if (list->tail)
    list->tail->next = node;
  else
    list->head = node;
  list->tail = node;

  list->size++;
}

void
list_insert_at (list_t *list, size_t index, list_node_t *node)
{
  size_t size = list->size;

  if (gcc_unlikely (index > size))
    return;

  if (index == size)
    return list_push_back (list, node);

  if (index == 0)
    return list_push_front (list, node);

  list_node_t *old = list->head;
  for (; index; index--)
    old = old->next;

  list_insert_front (list, old, node);
}

void
list_insert_front (list_t *list, list_node_t *pos, list_node_t *node)
{
  node->next = pos;
  node->prev = pos->prev;

  if (pos->prev)
    pos->prev->next = node;
  else
    list->head = node;
  pos->prev = node;

  list->size++;
}

void
list_insert_back (list_t *list, list_node_t *pos, list_node_t *node)
{
  node->prev = pos;
  node->next = pos->next;

  if (pos->next)
    pos->next->prev = node;
  else
    list->tail = node;
  pos->next = node;

  list->size++;
}

void
list_erase (list_t *list, list_node_t *node)
{
  list_node_t *prev = node->prev;
  list_node_t *next = node->next;

  if (node == list->head)
    list->head = next;
  if (node == list->tail)
    list->tail = prev;

  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;

  list->size--;
}

void
list_erase_at (list_t *list, size_t index)
{
  list_node_t *node;
  if ((node = list_at (list, index)))
    list_erase (list, node);
}

void
list_pop_front (list_t *list)
{
  list_node_t *node;
  if ((node = list->head))
    list_erase (list, node);
}

void
list_pop_back (list_t *list)
{
  list_node_t *node;
  if ((node = list->tail))
    list_erase (list, node);
}

/* **************************************************************** */
/*                               ext                                */
/* **************************************************************** */

list_node_t *
list_find (const list_t *list, const list_node_t *target, list_comp_t *comp)
{
  list_node_t *curr = list->head;

  for (size_t size = list->size; size; size--)
    {
      if (comp (target, curr) == 0)
        return curr;
      curr = curr->next;
    }

  return NULL;
}

void
list_for_each (list_t *list, list_visit_t *visit)
{
  list_node_t *curr = list->head, *next;

  for (size_t size = list->size; size; size--)
    {
      next = curr->next;
      visit (curr);
      curr = next;
    }
}
