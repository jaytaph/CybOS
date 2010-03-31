/******************************************************************************
 *
 *  File        : queue.c
 *  Description : Queue functionality
 *
 *****************************************************************************/
#include "queue.h"
#include "klib.h"

void queue_insert_item (QUEUE *queue, void *data) {
  kprintf ("queue_insert_item(%08X %08X)\n", queue, data);
  QUEUE_ITEM *item = (QUEUE_ITEM *)kmalloc (sizeof (QUEUE_ITEM));
  item->data = data;

  kprintf ("Item alloced on %08X\n", item);

  if (queue->start != NULL) {
    queue->start->prev = (void *)item;
    item->next = (void *)queue->start;
    item->prev = NULL;
  } else {
    item->next = NULL;
    item->prev = NULL;
  }

  queue->start = item;
  queue->count++;
}

void queue_add_item (QUEUE *queue, void *data) {
  kprintf ("queue_add_item(%08X %08X)\n", queue, data);
  QUEUE_ITEM *item = (QUEUE_ITEM *)kmalloc (sizeof (QUEUE_ITEM));

  kprintf ("Item alloced on %08X\n", item);
  item->data = data;

  if (queue->end != NULL) {
    queue->end->next = (void *)item;
    item->prev = (void *)queue->end;
    item->next = NULL;
  } else {
    item->next = NULL;
  }

  if (queue->start == NULL) queue->start = item;
  queue->end = item;
  queue->count++;
}


void queue_del_item (QUEUE *queue, void *data) {
  kprintf ("queue_del_item()\n");
/*
  if (queue->ptr == NULL) return;

  QUEUE_ITEM *tmp = queue->ptr;

  // Removing the start
  if (queue->ptr->prev == NULL && queue->ptr->next == NULL) {
    kree (queue->ptr);
    return;
  }

  if (queue->ptr->prev == NULL) {
    queue->ptr->next->prev = NULL;
  }


  if (queue->prev != NULL) {
  }

  queue->ptr = queue->ptr->next;
*/
}

void queue_clear (QUEUE *queue) {
  QUEUE_ITEM *item;

  kprintf ("queue_clear()\n");

  // Free memory for queue
  queue_reset (queue);
  while (item = queue_next (queue), item) kfree ((Uint32)item);

  queue->start = NULL;
  queue->end = NULL;
  queue->ptr = NULL;
  queue->count = 0;
}

void *queue_reset (QUEUE *queue) {
  kprintf ("queue_reset(%08X)\n", queue);
  queue->ptr = queue->start;

  kprintf ("%08X\n", queue->ptr->data);
  return queue->ptr->data;
}

void *queue_prev (QUEUE *queue) {
  kprintf ("queue_prev(%08X)\n", queue);
  queue->ptr = (void *)queue->ptr->prev;
  kprintf ("%08X\n", queue->ptr->data);
  return queue->ptr->data;
}

void *queue_next (QUEUE *queue) {
  kprintf ("queue_next(%08X)\n", queue);
  queue->ptr = (void *)queue->ptr->next;
  kprintf ("%08X\n", queue->ptr->data);
  return queue->ptr->data;
}

void *queue_end (QUEUE *queue) {
  kprintf ("queue_end(%08X)\n", queue);
  queue->ptr = queue->end;
  kprintf ("%08X\n", queue->ptr->data);
  return queue->ptr->data;
}

int queue_count (QUEUE *queue) {
  kprintf ("queue_count(%08X): %d\n", queue, queue->count);
  return queue->count;
}

