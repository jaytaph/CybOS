/******************************************************************************
 *
 *  File        : queue.h
 *  Description : Queue functionality
 *
 *****************************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "ktype.h"
#include "klib.h"
#include "kmem.h"

typedef struct QUEUE_ITEM {
  struct QUEUE_ITEM *prev;
  struct QUEUE_ITEM *next;
  void *data;
} QUEUE_ITEM;

typedef struct {
  QUEUE_ITEM *start;
  QUEUE_ITEM *end;
  QUEUE_ITEM *ptr;
  Uint32 count;
} QUEUE;

void queue_add_item (QUEUE *queue, void *data);       // Add new item to queue
void queue_del_item (QUEUE *queue, void *data);       // Remove specified queue item

void queue_clear (QUEUE *queue);      // Remove all items from queue

int queue_count (QUEUE *queue);      // Returns number of items in queue

void queue_reset (QUEUE *queue);      // Reset queue pointer to start
void queue_end (QUEUE *queue);       // Set pointer to end of queue

void *queue_get (QUEUE *queue);
void *queue_get_and_prev (QUEUE *queue);
void *queue_get_and_next (QUEUE *queue);

#endif // __QUEUE_H__
