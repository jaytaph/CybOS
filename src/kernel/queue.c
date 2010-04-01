/******************************************************************************
 *
 *  File        : queue.c
 *  Description : Queue functionality
 *
 *****************************************************************************/
#include "queue.h"
#include "klib.h"
#include "kernel.h"


/**
 * Insert item to the start of the queue
 */
void queue_insert_item (QUEUE *queue, void *data) {
//  kprintf ("queue_insert_item(%08X %08X)\n", queue, data);

  // Create queue item and add data to item
  QUEUE_ITEM *item = (QUEUE_ITEM *)kmalloc (sizeof (QUEUE_ITEM));
  item->data = data;

//  kprintf ("Item alloced on %08X\n", item);

  if (queue->start != NULL) {
    // queue not empty
    queue->start->prev = (void *)item;
    item->next = (void *)queue->start;
    item->prev = NULL;
  } else {
    // Queue empty, this is the first item
    item->next = NULL;
    item->prev = NULL;
  }

  // This item is the start as well
  if (queue->end == NULL) queue->end = item;
  queue->start = item;

  // Increase item count
  queue->count++;
}


/**
 * Add item to end of queue
 */
void queue_add_item (QUEUE *queue, void *data) {
//  kprintf ("queue_add_item(%08X %08X)\n", queue, data);

  // Create queue item
  QUEUE_ITEM *item = (QUEUE_ITEM *)kmalloc (sizeof (QUEUE_ITEM));
  item->data = data;

//  kprintf ("Item alloced on %08X\n", item);

  if (queue->end != NULL) {
    // queue not empty
    queue->end->next = (void *)item;
    item->prev = (void *)queue->end;
    item->next = NULL;
  } else {
    item->next = NULL;
    item->prev = NULL;
    queue->start = item;
  }

  if (queue->start == NULL) queue->start = item;
  queue->end = item;

  // increase item count
  queue->count++;
}


/**
 * Delete specified item from queue
 */
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

/**
 * Remove all items from the queue
 */
void queue_clear (QUEUE *queue) {
  QUEUE_ITEM *item;

//  kprintf ("queue_clear()\n");

  // Free memory for queue
  queue_reset (queue);
  while (item = queue_get_and_next (queue), item != NULL) kfree ((Uint32)item);

  queue->start = NULL;
  queue->end = NULL;
  queue->ptr = NULL;
  queue->count = 0;
}


/**
 * Reset queue ptr to start and return first item
 */
void queue_reset (QUEUE *queue) {
//  kprintf ("queue_reset(%08X)\n", queue);
  queue->ptr = queue->start;
}


/**
 * Return current item in queue and decrease
 */
void *queue_get_and_prev (QUEUE *queue) {
//  kprintf ("queue_prev(%08X)\n", queue);
  if (queue->ptr == NULL) return NULL;

  void *cur_item = queue->ptr->data;

  // Decrease pointer
  queue->ptr = (void *)queue->ptr->prev;

  return cur_item;
}

/**
 * Return current item in queue and increase
 */
void *queue_get_and_next (QUEUE *queue) {
//  kprintf ("queue_get_and_next(%08X)\n", queue);
  if (queue->ptr == NULL) return NULL;

  void *cur_item = queue->ptr->data;

  // Increase pointer
  queue->ptr = (void *)queue->ptr->next;

  return cur_item;
}

/**
 * Return next item in queue or NULL on end
 */
void *queue_get (QUEUE *queue) {
//  kprintf ("queue_next(%08X)\n", queue);
  if (queue->ptr == NULL) return NULL;

  return queue->ptr->data;
}


/**
 * Return end of the queue (and set queue ptr)
 */
void queue_end (QUEUE *queue) {
//  kprintf ("queue_end(%08X)\n", queue);
  queue->ptr = queue->end;
}


/**
 * Return queue count
 */
int queue_count (QUEUE *queue) {
//  kprintf ("queue_count(%08X): %d\n", queue, queue->count);
  return queue->count;
}

