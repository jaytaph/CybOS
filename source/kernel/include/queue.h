/******************************************************************************
 *
 *  File        : queue.h
 *  Description : Queues (or double linked lists)
 *
 *****************************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct queue_item {
  struct queue_item *prev;
  struct queue_item *next;
  void *data;
} queue_item_t;

typedef struct queue {
  queue_item_t *head;
  queue_item_t *tail;
  int count;
} queue_t;



queue_t *queue_init ();
queue_item_t *queue_seek (queue_t *queue, int (*callback)(void *));
void queue_destroy (queue_t *queue, void (*callback)(void *));
int queue_append (queue_t *queue, void *data);
int queue_insert (queue_t *queue, void *data, queue_item_t *pre_item);
int queue_insert_noalloc (queue_t *queue, queue_item_t *pre_item, queue_item_t *item);
int queue_prepend (queue_t *queue, void *data);
int queue_remove (queue_t *queue, queue_item_t *item);
queue_item_t *queue_iterate (queue_t *queue, queue_item_t *cur_item);

#endif //__QUEUE_H__
