/******************************************************************************
 *
 *  File        : queue.c
 *  Description : Queues (or double linked lists)
 *
 *****************************************************************************/
#include "queue.h"
#include "kernel.h"
#include "kmem.h"

/**
 *
 */
queue_t *queue_init () {
  // Alloc memory for queue
  queue_t *queue = (queue_t *)kmalloc (sizeof (queue_t));

  // Init queue
  queue->count = 0;
  queue->head = NULL;
  queue->tail = NULL;

  return queue;
}


/**
 * Destroys whole queue while running callback() on east queue_item->data
 */
void queue_destroy (queue_t *queue, void (*callback)(void *)) {

  while (queue->count) {
    queue_item_t *item = queue->tail;

    if (callback) callback (item->data);
    queue_remove (queue, item);
  }

  kfree (queue);
}


/**
 * Seek item in the queue. Needs a callback in order to check if the
 * item is the one we seek.
 */
queue_item_t *queue_seek (queue_t *queue, int (*callback)(void *)) {
  queue_item_t *item = queue->head;

//  kprintf ("queue_seek()\n");

  // No correct callback
  if (callback == NULL) return NULL;

  while (item) {
//    kprintf ("queue_seek(): item  prev: %08X  next: %08X\n", item->prev, item->next);

    if (callback(item->data)) return item;

//    kprintf ("queue_seek(): next item\n");
    item = item->next;
  }

  return NULL;
}


/**
 * Inserts data AFTER queue_item. When dst is NULL, it adds to the head
 * of the queue. Note that item here MUST be already allocated. This is needed
 * in some situations in the VMM, where we are calling alloc() inside the alloc(),
 * which does not work when all the pointers are laying open.
 *
 */
int queue_insert_noalloc (queue_t *queue, queue_item_t *pre_item, queue_item_t *item) {
//  kprintf ("queue_insert_noalloc\n");

  // Increase queue count
  queue->count++;

  /*
   * 4 scenario's: (1) first entry, (2) adding to head, (3) adding to tail and (4) adding in between
   */

  // 1: First item?
  if (queue->head == NULL) {
    queue->head = item;
    queue->tail = item;
    item->next = NULL;
    item->prev = NULL;
    return 1;
  }

  // 2: Add item to the head of the queue?
  if (pre_item == NULL) {
//    kprintf ("Adding to head\n");
    item->next = queue->head;
    item->prev = NULL;
    queue->head->prev = item;
    queue->head = item;
//    kprintf ("queue head is now: %08X\n", queue->head);
    return 1;
  }

  // 3: Item to be added after the tail of the queue?
  if (pre_item == queue->tail) {
//    kprintf ("Adding after tail\n");
    item->next = NULL;
    item->prev = queue->tail;
    queue->tail->next = item;
    queue->tail = item;
    return 1;
  }

  // 4: Somewhere in between then
//  kprintf ("Adding in between \n");
  item->next = pre_item->next;
  item->prev = pre_item;
  pre_item->next->prev = item;
  pre_item->next = item;
  return 1;
}


/**
 * Inserts data AFTER queue_item. When dst is NULL, it adds to the head
 * of the queue.
 */
int queue_insert (queue_t *queue, void *data, queue_item_t *pre_item) {
//  kprintf ("queue_insert\n");
  // Allocate queue item memory
  queue_item_t *item = (queue_item_t *)kmalloc (sizeof(queue_item_t));
  item->data = data;

  return queue_insert_noalloc (queue, pre_item, item);
}


/**
 * Add to the tail of the queue
 */
int queue_append (queue_t *queue, void *data) {
//  kprintf ("queue_append\n");
  return queue_insert (queue, data, queue->tail);
}


/**
 * Add to the head of the queue
 */
int queue_prepend (queue_t *queue, void *data) {
//  kprintf ("queue_prepend\n");
  return queue_insert (queue, data, NULL);
}


/**
 * Removes queue item from the queue
 */
int _queue_remove (queue_t *queue, queue_item_t *item, char free_item) {
  queue_item_t *prev_item;
  queue_item_t *next_item;

  // Nothing here
  if (queue->count == 0) return 0;

  prev_item = item->prev;
  next_item = item->next;

  // @TODO: somewhere here needs to update queue->head and queue->tail

  if (item == queue->head) {
    // Remove head of queue
    queue->head = next_item;
    queue->head->prev= NULL;
  } else if (item == queue->tail) {
    // Remove tail of queue
    queue->tail = prev_item;
    queue->tail->next = NULL;
  } else {
    // Remove something in the middle
    prev_item->next = next_item;
    next_item->prev = prev_item;
  }

  queue->count--;

  // Only free the queue item if we need to
  if (free_item) kfree (item);

  return 1;
}


/**
 *
 */
int queue_remove (queue_t *queue, queue_item_t *item) {
  return _queue_remove (queue, item, 1);
}

/**
 *
 */
int queue_remove_noalloc (queue_t *queue, queue_item_t *item) {
  return _queue_remove (queue, item, 0);
}

/**
 * Iterates a queue. If current_item is NULL it will start from the head. Returns NULL
 * after the last item
 */
queue_item_t *queue_iterate (queue_t *queue, queue_item_t *current_item) {
  if (current_item == NULL) return queue->head;
  return current_item->next;
}
