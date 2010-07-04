/******************************************************************************
 *
 *  File        : heap.c
 *  Description : Heap thingies
 *
 *****************************************************************************/
#include "kernel.h"
#include "heap.h"
#include "kmem.h"
#include "errors.h"
#include "paging.h"
#include "queue.h"

heap_t *kheap;      // Kernel heap

char vmm_debug = 0;

/*
 * @TODO: We should not use heap->end to check to see if it's the last header since
 * we could have additional space there
 */


void print_heap_info (heap_t *heap);


/**
 * callback function for ordering queues
 */
static Uint32 _heap_callback_ordered_var;
int _heap_callback_ordered (void *a) {
  int ret;
  if (vmm_debug) kprintf ("Heap callback: is %X less than %X ==> ", _heap_callback_ordered_var, ((heap_header_t *)a)->size);
  ret = _heap_callback_ordered_var > ((heap_header_t *)a)->size ? 1 : 0;
  if (vmm_debug) kprintf (" %d\n", ret);
  return ret;
}


/**
 * callback function to find specified header
 */
static Uint32 _heap_callback_findheader_var;
int _heap_callback_findheader (void *a) {
  return ((Uint32)a == _heap_callback_findheader_var);
}


/**
 * inserts item in ordered queue
 */
void heap_insert_in_ordered_queue (queue_t *queue, heap_header_t *heap_header) {
  if (vmm_debug) kprintf ("HIIOQ: heap_insert_in_ordered_queue(size: %d 0x%X)\n", heap_header->size, heap_header->size);
  queue_item_t *pre_item = NULL;

  // queue_item is part of the heap header, so we do not need to allocate it
  heap_header->queue_item.data = heap_header;

  // Do not seek when we do not have anything present yet..
  if (queue->head != NULL) {
    // Do not seek when this item belongs before the first entry..
    if (heap_header->size > ((heap_header_t *)queue->head)->size) {
      _heap_callback_ordered_var = heap_header->size;
      pre_item = queue_seek (queue, _heap_callback_ordered);

      // No entry found, this item must be after the last item
      if (pre_item == NULL) {
        pre_item = queue->tail;
        if (vmm_debug) kprintf ("HIIOQ: Adding to tail\n");
      } else {
        /* We have found the item AFTER the one we need to add, goto previous item. This
         * also works correctly on the first entry */
        pre_item = pre_item->prev;
     }
    }
  } else {
    if (vmm_debug) kprintf ("HIIOQ: First entry, added to head (NULL)\n");
  }

  if (vmm_debug) kprintf ("HIIOQ: Adding entry after %08X\n", pre_item);
  queue_insert_noalloc (queue, pre_item, &heap_header->queue_item);
}

/**
 *
 */
heap_t *heap_create (Uint32 start, Uint32 end, Uint32 max, Uint32 extend, char supervisor, char readonly) {
  heap_t *heap = (heap_t *)kmalloc (sizeof (heap_t));

  heap->queue = queue_init ();

  // Align start address on page boundary
  if ( (start & 0xFFF) != 0) {
    start &= 0xFFFFF000;
    start += 0x1000;
  }

  // Set data
  heap->start = start;
  heap->end = end;
  heap->max = max;
  heap->extend = extend;
  heap->supervisor = supervisor;
  heap->readonly = readonly;


  // Create initial hole block in heap
  heap_header_t *hole_header = (heap_header_t *)start;
  hole_header->size = end - start;
  hole_header->magic = HEAP_HEADER_MAGIC;
  hole_header->is_hole = 1;

  heap_footer_t *hole_footer = (heap_footer_t *)((Uint32)start + hole_header->size - sizeof (heap_footer_t));
  hole_footer->magic = HEAP_FOOTER_MAGIC;
  hole_footer->header = hole_header;

  heap_insert_in_ordered_queue (heap->queue, hole_header);

  return heap;
}

/************************************************************************
 * Initialize the kernel memory manager.
 */
int heap_init (void) {
  int i;

  // Here we allocate the memory for the heap.
  for (i=HEAP_START; i < HEAP_START + MIN_HEAP_SIZE; i+=0x1000) {
    create_pageframe (_kernel_pagedirectory, i, PAGEFLAG_USER + PAGEFLAG_PRESENT + PAGEFLAG_READWRITE);
    // @TODO: create_pageframe (_kernel_pagedirectory, i, PAGEFLAG_SUPERVISOR + PAGEFLAG_PRESENT + PAGEFLAG_READWRITE);
  }

  // We just added the heap frames stuff. Flush pagedirectory
  flush_pagedirectory ();

  // Create kernel heap
  kheap = heap_create (HEAP_START, HEAP_START+MIN_HEAP_SIZE, MAX_HEAP_SIZE, HEAP_EXTEND, 0, 0);

  // All done..

  // Switch to new malloc system
  if (vmm_debug) kprintf ("Switching to new malloc system");
  kmem_switch_malloc (_heap_kmalloc, _heap_kfree);


/*
  kprintf ("Doing actual heap malloc now...\n");
  int *a = (int *)kmalloc (9);
  kprintf ("A: %08X\n", a);

  print_heap_info (kheap);

  int *b = (int *)kmalloc (8);
  kprintf ("B: %08X\n", b);

  print_heap_info (kheap);

  int *c = (int *)kmalloc (10);
  kprintf ("C: %08X\n", c);

  print_heap_info (kheap);

  int *d = (int *)kmalloc (5);
  kprintf ("D: %08X\n", d);

  print_heap_info (kheap);

  kprintf ("A ( 9) : %08X\n", a);
  kprintf ("B ( 8) : %08X\n", b);
  kprintf ("C (10) : %08X\n", c);
  kprintf ("D ( 5) : %08X\n", d);

  kprintf ("Freeing A\n");

  kfree (b);

  print_heap_info (kheap);

  kfree (a);

  print_heap_info (kheap);

  for (;;);

  kfree (b);
  int *z = (int *)kmalloc (12);
  kprintf ("Z: %08X\n", z);
*/

  return ERR_OK;
}


/**
 *
 */
queue_item_t *heap_find_smallest_hole (Uint32 size, Uint8 aligned, heap_t *heap) {
  if (vmm_debug) kprintf ("heap_find_smallest_hole (size: %d, aligned: %d, heap: %08X\n", size, aligned, heap);

  // Iterate over all
  queue_item_t *item = NULL;
  while (item = queue_iterate (heap->queue, item), item) {
    heap_header_t *header = item->data;
    if (vmm_debug) kprintf ("iterating header: %08X\n", header);
    if (header->is_hole == 0) continue;
    if (vmm_debug) kprintf ("IsHole\n");

    if (aligned > 0) {
      // Must be aligned, NOTE: the DATA must be aligned, not the actual header before the data
      Uint32 location = (Uint32)header;
      Sint32 offset = 0;
      if (((location + sizeof (heap_header_t)) & 0xFFFFF000) != 0) {
        offset = 0x1000 - (location+sizeof (heap_header_t)) % 0x1000;
      }
      Sint32 hole_size = (Sint32)header->size - offset;
      if (hole_size >= (Sint32)size) return item;
    } else if (header->size >= size) {
      // It's ok
      return item;
    }
  }

  if (vmm_debug) kprintf ("heap_find_smallest_hole: nothing found !???\n");
  return NULL;
}


/**
 *
 */
int heap_expand (heap_t *heap, Uint32 new_size) {
  if (vmm_debug) kprintf ("heap_expand (%d)\n", new_size);

  // Must be in range
  if (new_size > heap->end - heap->start) return 0;

  // We need to extend to at least this amount, otherwise it gets too costly to
  // expand for only a few bytes etc..
  if (new_size > heap->extend) new_size += heap->extend;

  // Page align new size if needed
  if ( (new_size & 0xFFFFF000) != 0) {
    new_size &= 0xFFFFF000;
    new_size += 0x1000;
  }

  // Heap cannot be larger than max
  if (heap->start + new_size <= heap->max) {
    new_size = heap->max - heap->start;
  }

  Uint32 old_size = heap->end - heap->start;
  Uint32 i = old_size;
  while (i < new_size) {
    allocate_pageframe( get_pageframe (heap->start+i, 1, _kernel_pagedirectory), (heap->readonly)?0:1, (heap->supervisor)?1:0);
    i += 0x1000;
  }

  heap->end = heap->start + new_size;

  return new_size;
}


/**
 *
 */
Uint32 heap_contract (heap_t *heap, Uint32 new_size) {
  if (new_size < heap->end - heap->start) return 0;

  if (new_size & 0x1000) {
    // @TODO: is this right? Must this not be 0xFFFFF000?
    new_size &= 0x1000;
    new_size += 0x1000;
  }

  if (new_size < MIN_HEAP_SIZE) new_size = MIN_HEAP_SIZE;

  Uint32 old_size = heap->end - heap->start;
  Uint32 i = old_size - 0x1000;
  while (new_size < i) {
    free_pageframe (get_pageframe (heap->start+i, 0, _kernel_pagedirectory));
    i -= 0x1000;
  }

  return new_size;
}


/**
 *
 */
void *_heap_alloc (Uint32 size, Uint32 aligned, heap_t *heap) {
  if (vmm_debug) kprintf ("_HA: _heap_alloc(%d, %d, %08X)\n", size, aligned, heap);

//  print_heap_info (heap);

  // The actual reserved size is including a heap header and footer
  Uint32 new_size = size + sizeof (heap_header_t) + sizeof (heap_footer_t);

  // Find a hole-block that can handle this new size
  queue_item_t *orig_hole = heap_find_smallest_hole (new_size, aligned, heap);

  if (vmm_debug) kprintf ("_HA: I have found a hole that matches: %08X\n", orig_hole);


  if (orig_hole == NULL) {
    if (vmm_debug) kprintf ("_HA: Did not find a suitable hole, expand the heap a bit\n");
    Uint32 old_length = heap->end - heap->start;
    Uint32 old_end = heap->end;

    if (vmm_debug) kprintf ("_HA: Expanding the heap with %d bytes\n", old_length + new_size);
    Uint32 expansion = heap_expand (heap, old_length + new_size);

    Uint32 new_length = heap->end - heap->start;
    if (vmm_debug) kprintf ("_HA: New size is %d, expanded %d bytes (could be more than we asked for, no problem)\n", new_length, expansion);


    if (heap->queue->count == 0) {
      if (vmm_debug) kprintf ("_HA: This is the first block we're adding\n");

      // Creating header
      heap_header_t *header = (heap_header_t *)old_end;
      header->magic = HEAP_HEADER_MAGIC;
      header->size = expansion;
      header->is_hole = 1;

      // Creating footer
      heap_footer_t *footer = (heap_footer_t *)old_end + header->size - sizeof(heap_footer_t);
      footer->magic = HEAP_FOOTER_MAGIC;
      footer->header = header;

      if (vmm_debug) kprintf ("_HA: Adding block into the new queue\n");
      heap_insert_in_ordered_queue (heap->queue, header);
    } else {

      // The last block just need to be adjusted because the heap is expanded.
      heap_header_t *last_header = (heap_header_t *)((heap_footer_t *)heap->end)->header;
      Uint32 old_size = last_header->size;
      last_header->size = expansion;

      if (vmm_debug) kprintf ("_HA: Adjusting the last block (%08X) (size from %d to %d)\n", (Uint32)last_header, old_size, expansion);

      // Remove from queue and add again, just in case the ordering should have changed
      queue_remove_noalloc (heap->queue, &last_header->queue_item);
      heap_insert_in_ordered_queue (heap->queue, last_header);

      /* Since the footer is on the wrong place, we need to create a new footer as well. Don't care about the
       * old footer. That will be overwritten by other data once that part gets allocated */
      heap_footer_t *last_footer = (heap_footer_t *)( (Uint32)last_header + last_header->size - sizeof (heap_footer_t));
      last_footer->header = last_header;
      last_footer->magic = HEAP_FOOTER_MAGIC;
    }

    // Enough room now. allocate block
    if (vmm_debug) kprintf ("_HA: _heap_alloc() done: we can try again to allocate the memory (recursivly)\n");
    return _heap_alloc (size, aligned, heap);
  }


  if (vmm_debug) kprintf ("_HA: We have found a block that could hold our new alloc, see if we need to split\n");

  // Save original hole values
  heap_header_t *orig_hole_header = (heap_header_t *)orig_hole->data;
  Uint32 orig_hole_pos = (Uint32)orig_hole->data;
  Uint32 orig_hole_size = orig_hole_header->size;

  // See if we need to split the hole block
  if (orig_hole_size - new_size < sizeof (heap_header_t)+sizeof (heap_footer_t)) {
    // @TODO: Isn't this ALWAYS the case??? What when it's just enough room!
    // @TODO: TEST THIS
    if (vmm_debug) kprintf ("_HA: This block can hold our new alloc\n");

    // No need, just adjust values
    size += orig_hole_size - new_size;
    new_size = orig_hole_size;
  }


  // Delete hole from index since it will become a data block
  if (vmm_debug) kprintf ("_HA: Removing hole %08X block from the queue since it will become a datablock\n", orig_hole);
  queue_remove_noalloc (heap->queue, orig_hole);


  // Page align the block? Create small hole block in front of it
  if (aligned && orig_hole_pos & 0xFFFFF000) {
    /* @TODO: after the page alignment, do we have enough space to add the block?? It's something
     * we have checked above so it might be wrong */

    if (vmm_debug) kprintf ("_HA: Aligning block to next page-position\n");
    Uint32 new_location = orig_hole_pos + 0x1000 - (orig_hole_pos & 0xFFF) - sizeof (heap_header_t);
    if (vmm_debug) kprintf ("_HA: since a header has a certain size, it is placed misaligned (so the data IS aligned). It's set to: %08X\n", new_location);

    if (vmm_debug) kprintf ("_HA: we create a small hole in front of it, since we are about to adjusted the block\n");
    heap_header_t *hole_header = (heap_header_t *)orig_hole_pos;
    hole_header->size = 0x1000 - (orig_hole_pos & 0xFFF) - sizeof (heap_header_t);
    hole_header->magic = HEAP_HEADER_MAGIC;
    hole_header->is_hole = 1;

    heap_footer_t *hole_footer = (heap_footer_t *)( new_location - sizeof (heap_footer_t));
    hole_footer->magic = HEAP_FOOTER_MAGIC;
    hole_footer->header = hole_header;

    if (vmm_debug) kprintf ("_HA: insert new hole block into queue\n");
    heap_insert_in_ordered_queue (heap->queue, hole_header);

    // The new position of the datablock is this...
    orig_hole_pos = new_location;
    orig_hole_size = orig_hole_size - hole_header->size;
  }

  if (vmm_debug) kprintf ("_HA: we create a new data block header based on the new position %08X. It size: %d\n", orig_hole_pos, new_size);
  // Create new data block on correct position
  heap_header_t *block_header = (heap_header_t *)orig_hole_pos;
  block_header->magic = HEAP_HEADER_MAGIC;
  block_header->is_hole = 0;  // it's a data block
  block_header->size = new_size;

  // Create footer for this data block
  heap_footer_t *block_footer = (heap_footer_t *)(orig_hole_pos + sizeof (heap_header_t) + size);
  block_footer->magic = HEAP_FOOTER_MAGIC;
  block_footer->header = block_header;

  // Should we create a new hole block after the allocated block?
  if (orig_hole_size - new_size > 0) {
    if (vmm_debug) kprintf ("_HA: There is still room after the block, we need to place a hole block here\n");

    // Create hole block AFTER the block
    heap_header_t *hole_header = (heap_header_t *)(orig_hole_pos + sizeof (heap_header_t)+ size + sizeof (heap_footer_t));
    hole_header->magic = HEAP_HEADER_MAGIC;
    hole_header->is_hole = 1;
    hole_header->size = orig_hole_size - new_size;
    if (vmm_debug) kprintf ("_HA: hole block added to position %08X with size %d\n", (Uint32)hole_header, hole_header->size);

    heap_footer_t *hole_footer = (heap_footer_t *)((Uint32)hole_header + orig_hole_size - new_size - sizeof (heap_footer_t));

    // Is the block still in range, otherwise don't add (@TODO: is this correct?)
    if ((Uint32)hole_footer < heap->end) {
      hole_footer->magic = HEAP_FOOTER_MAGIC;
      hole_footer->header = hole_header;
    } else {
      /* @TODO: This should not happen. If we do NOT have enough room, we should not be able to
       * add data here.. It might cause problems when a header does not have a footer */
    }

    if (vmm_debug) kprintf ("_HA: insert hole-header in the queue\n");
    heap_insert_in_ordered_queue (heap->queue, hole_header);
  }

  // Return a pointer to the data block we have allocated\n");
  return (void *)( (Uint32)block_header + sizeof (heap_header_t) );
}


/**
 *
 */
void _heap_free (heap_t *heap, void *ptr) {
  if (vmm_debug) kprintf ("_HF: heap: %08X, p: %08X\n", heap, ptr);
  if (ptr == 0) return;

//  print_heap_info (heap);

  heap_header_t *header = (heap_header_t *)( (Uint32)ptr - sizeof (heap_header_t) );
  heap_footer_t *footer = (heap_footer_t *)( (Uint32)header + header->size - sizeof (heap_footer_t));

  // Sanity checks to see if this really is a block...
  if (header->magic != HEAP_HEADER_MAGIC) return;
  if (footer->magic != HEAP_FOOTER_MAGIC) return;

  // This block is free again..
  header->is_hole = 1;

  /* By default, we cannot merge this block, so we need to add it. But when we have unified it,
   * there is no need to add it anomore. Instead of adding it now, to get it removed a few lines
   * below, we just set a flag to add it later. If the block gets unified, we set the do_add
   * to 0 */
  char do_add = 1;


  // Unify left
  if (vmm_debug) kprintf ("_HF: Can we merge to the left?\n");
  heap_footer_t *prev_footer = (heap_footer_t *)( (Uint32)header - sizeof (heap_footer_t));

  // Only merge left when this is NOT the first block!
  if ((Uint32)header != heap->start) {
    /* Check if the previous header is a hole, if so, we can "merge" the current block and
     * the previous one */
    if (prev_footer->magic == HEAP_FOOTER_MAGIC && prev_footer->header->is_hole == 1) {
      if (vmm_debug) kprintf ("_HF: merging current with previous block on %08X\n", prev_footer->header);

      // Do actual merge
      footer->header = prev_footer->header;
      prev_footer->header->size += header->size;

      // Since the current header is removed, we can remove it also from the queue
      queue_remove_noalloc (heap->queue, &header->queue_item);

      // We do not need to add the block since it's merged..
      do_add = 0;
    }
  } else {
    if (vmm_debug) kprintf ("_HF: This is the first block, no merge left is possible!\n");
  }

  // Unify right
  if (vmm_debug) kprintf ("_HF: Can we merge to the right?\n");
  if ((Uint32)header + header->size != heap->end) {  // @TODO: Don't use heap->end to detect last header
    /* Check if the next header is a hole, if so, we can "merge" the current block and the
     * next one */
    heap_header_t *next_header = (heap_header_t *)( (Uint32)footer + sizeof (heap_footer_t));
    if (next_header->magic == HEAP_HEADER_MAGIC && next_header->is_hole == 1) {
      if (vmm_debug) kprintf ("_HF: merging current with next block on %08X\n", next_header);

      // Do actual merge
      header->size += next_header->size;

      // Since the current header is removed, we can remove it also from the queue
      queue_remove_noalloc (heap->queue, &next_header->queue_item);

//      // Find and remove obsolete header
//      _heap_callback_findheader_var = (Uint32)next_header;
//      queue_item_t *item = queue_seek (heap->queue, _heap_callback_findheader);
//      if (item) queue_remove_noalloc (heap->queue, item);
    }
  } else {
    if (vmm_debug) kprintf ("_HF: This is the last block, no merge right is possible!\n");
  }


  // Is this footer the end of the heap, we can contract the heap
  if ((Uint32)header + header->size == heap->end) {   // @TODO: Don't use heap->end to detect last header
    if (vmm_debug) kprintf ("_HF: We might be able to contract the heap now...\n");
    Uint32 old_length = heap->end - heap->start;
    Uint32 new_length = heap_contract (heap, (Uint32)header - heap->start);

    if (vmm_debug) kprintf ("_HF: Old length: %d\n", old_length);
    if (vmm_debug) kprintf ("_HF: New length: %d\n", new_length);

    // Is this whole block contracted?
    if (header->size - (old_length - new_length) > 0) {
      if (vmm_debug) kprintf ("_HF: the block is still a bit visible: adjust size and create footer\n");

      // Adjust block size
      header->size -= old_length - new_length;

      // Create new footer
      footer = (heap_footer_t *)( (Uint32)header + header->size - sizeof (heap_footer_t));
      footer->magic = HEAP_FOOTER_MAGIC;
      footer->header = header;
    } else {
      if (vmm_debug) kprintf ("_HF: this block is not visible. Remove complete block\n");
      queue_remove_noalloc (heap->queue, &header->queue_item);

      // @TODO: what's happening here. Is there a free piece of heap at the end now????
    }
  }

  if (do_add == 1) {
    if (vmm_debug) kprintf ("_HF: Adding header since this is a hole block again\n");
    heap_insert_in_ordered_queue (heap->queue, header);
  }
}


/**
 *
 */
void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address) {
  kprintf ("kmalloc (%d, %d)\n", size, pageboundary);
  if (size == 0) return NULL;

  void *ptr = _heap_alloc (size, pageboundary, kheap);

  // We know the virtual address, we can lookup the physical address in the _kernel_pagedirectory
  if (physical_address != NULL) {
    (*physical_address) = get_physical_address (_kernel_pagedirectory, (Uint32)ptr);
  }

  return ptr;
}


// ========================================================
// Frees a block
void _heap_kfree (void *ptr) {
  kprintf ("kmalloc (%08X)\n", ptr);
  _heap_free (kheap, ptr);
}


/**
 *
 */
void print_heap_info (heap_t *heap) {
  kprintf ("==================== Heap info for heap %08X =====================\n", heap);
  kprintf ("\n");

  kprintf ("Heap start   : %08X\n", heap->start);
  kprintf ("Heap end     : %08X\n", heap->end);
  kprintf ("Heap max     : %08X\n", heap->max);
  kprintf ("Current size : %08X\n", heap->end - heap->start);
  kprintf ("\n");
  kprintf ("\n");

  kprintf ("Holeblock queue info (%d items):\n", heap->queue->count);
  queue_item_t *item = NULL;
  while (item = queue_iterate (heap->queue, item), item) {
    heap_header_t *header = (heap_header_t *)item->data;
    kprintf ("P: %08X    M: %08X  T:%c  S:%08d (0x%08X)\n",
    header, header->magic, (header->is_hole?'H':'B'), header->size, header->size);
  }
  kprintf ("\n");
  kprintf ("\n");

  kprintf ("Link info:\n");
  Uint32 header_ptr = heap->start;
  while (header_ptr < heap->end) {
    heap_header_t *header = (heap_header_t *)header_ptr;
    heap_footer_t *footer = (heap_footer_t *)(header_ptr + header->size - sizeof (heap_footer_t));

    kprintf ("H:%08X  T:%c  S:%08d (0x%08X) M: %08X     F:%08X  M:%08X  H:%08X\n",
             header, (header->is_hole?'H':'B'), header->size, header->size, header->magic,
             footer, footer->magic, footer->header);
/*
    kprintf ("Block header: %08X  (end: %08X)\n", header, heap->end);
    kprintf ("  Magic   : %08X\n", header->magic);
    kprintf ("  Size    : %08X\n", header->size);
    kprintf ("  is_hole : %08X\n", header->is_hole);

    kprintf (" Block footer: %08X\n", footer);
    kprintf ("  Magic   : %08X\n", footer->magic);
    kprintf ("  header  : %08X\n", footer->header);
*/

    // Next header
    header_ptr += header->size;
  }

  kprintf ("\n======================================================================\n");
  kprintf ("\n\n");
}