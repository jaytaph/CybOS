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


void print_heap_info (heap_t *heap);


/**
 * callback function for ordering queues
 */
static Uint32 _heap_callback_ordered_var;
int _heap_callback_ordered (void *a) {
  return ((heap_header_t *)a)->size < _heap_callback_ordered_var ? 1 : 0;
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
void heap_insert_in_ordered_queue (queue_t *queue, heap_header_t *heap_header, queue_item_t *item) {
  _heap_callback_ordered_var = heap_header->size;
  queue_item_t *pre_item = queue_seek (queue, _heap_callback_ordered);
  if (item) {
    // queue_item was pre-allocated, so we do not need to allocate it again
    item->data = heap_header;   // But we need to set the data manually
    queue_insert_noalloc (queue, pre_item, item);
  } else {
    queue_insert (queue, heap_header, pre_item);
  }
}

/**
 *
 */
heap_t *heap_create (Uint32 start, Uint32 end, Uint32 max, char supervisor, char readonly) {
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
  heap->supervisor = supervisor;
  heap->readonly = readonly;


  // Create initial hole block in heap
  heap_header_t *hole_header = (heap_header_t *)start;
  hole_header->size = end - start;
  hole_header->magic = HEAP_MAGIC;
  hole_header->is_hole = 1;

  heap_footer_t *hole_footer = (heap_footer_t *)((Uint32)start + hole_header->size - sizeof (heap_footer_t));
  kprintf ("HEAP_FOOTER: %08X\n", hole_footer);
  hole_footer->magic = HEAP_MAGIC;
  hole_footer->header = hole_header;

  heap_insert_in_ordered_queue (heap->queue, hole_header, NULL);

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
  kheap = heap_create (HEAP_START, HEAP_START+MIN_HEAP_SIZE, MAX_HEAP_SIZE, 0, 0);
//  print_heap_info (kheap);

  // All done..

  // Switch to new malloc system
  kprintf ("Switching to new malloc system");
  kmem_switch_malloc (_heap_kmalloc, _heap_kfree);

  kprintf ("Doing actual heap malloc now...\n");
  int *a = (int *)kmalloc (8);
  kprintf ("A: %08X\n", a);
  print_heap_info (kheap);
  for (;;);

  for (;;);
  int *b = (int *)kmalloc (8);
  kprintf ("B: %08X\n", b);
  kfree (a);
  kfree (b);
  int *c = (int *)kmalloc (12);
  kprintf ("C: %08X\n", c);

  print_heap_info (kheap);


  return ERR_OK;
}


/**
 *
 */
queue_item_t *heap_find_smallest_hole (Uint32 size, Uint8 aligned, heap_t *heap) {
kprintf ("heap_find_smallest_hole (size: %d, aligned: %d, heap: %08X\n", size, aligned, heap);

  // Iterate over all
  queue_item_t *item = NULL;
  while (item = queue_iterate (heap->queue, item), item) {
    heap_header_t *header = item->data;
    kprintf ("iterating header: %08X\n", header);

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

kprintf ("heap_find_smallest_hole: nothing found !???\n");
  return NULL;
}


/**
 *
 */
void heap_expand (heap_t *heap, Uint32 new_size) {
  // Must be in range
  if (new_size > heap->end - heap->start) return;

  // We need to extend to at least this amount, otherwise it gets too costly to expand for only
  // a few bytes etc..
  if (new_size > HEAP_EXTEND) new_size += HEAP_EXTEND;

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
  kprintf ("_heap_alloc(%d, %d, %08X)\n", size, aligned, heap);

  print_heap_info (heap);

  Uint32 new_size = size + sizeof (heap_header_t) + sizeof (heap_footer_t);
  queue_item_t *orig_hole = heap_find_smallest_hole (new_size, aligned, heap);

  kprintf ("ORIG HOLE: %08X\n", orig_hole);


kprintf("Stage 1\n");

  if (orig_hole == NULL) {
    Uint32 old_length = heap->end - heap->start;
    Uint32 old_end = heap->end;

kprintf("Stage 1.1\n");
    heap_expand (heap, old_length + new_size);
kprintf("Stage 1.2\n");
    Uint32 new_length = heap->end - heap->start;

    // @TODO: huh?
    /*
       This is the original code:

       // Find the endmost header. (Not endmost in size, but in location).
       iterator = 0;
       // Vars to hold the index of, and value of, the endmost header found so far.
       u32int idx = -1; u32int value = 0x0;
       while (iterator < heap->index.size)
       {
           u32int tmp = (u32int)lookup_ordered_array(iterator, &heap->index);
           if (tmp > value)
           {
               value = tmp;
               idx = iterator;
           }
           iterator++;
       }
     */
    if (heap->queue->count == 0) {
kprintf("Stage 1.3\n");
      // No header found, add one
      heap_header_t *header = (heap_header_t *)old_end;
      header->magic = HEAP_MAGIC;
      header->size = new_length - old_length;
      header->is_hole = 1;

kprintf("Stage 1.4\n");
      heap_footer_t *footer = (heap_footer_t *)old_end + header->size - sizeof(heap_footer_t);
      footer->magic = HEAP_MAGIC;
      footer->header = header;

kprintf("Stage 1.5\n");
      heap_insert_in_ordered_queue (heap->queue, header, NULL);
    } else {
kprintf("Stage 1.6\n");
      // Just need adjusting
      heap_header_t *header = (heap_header_t *)orig_hole->data;
      header->size += new_length - old_length;

      heap_footer_t *footer = (heap_footer_t *)( (Uint32)header + header->size - sizeof (heap_footer_t));
      footer->header = header;
      footer->magic = HEAP_MAGIC;
    }

kprintf("Stage 1.7\n");
    // Enough room now. allocate block
    kprintf ("_heap_alloc() done\n");
    return _heap_alloc (size, aligned, heap);
  }


kprintf("Stage 2\n");

  // See if we need to split the hole block
  heap_header_t *orig_hole_header = (heap_header_t *)orig_hole->data;
  Uint32 orig_hole_pos = (Uint32)orig_hole->data;
  Uint32 orig_hole_size = orig_hole_header->size;

  if (orig_hole_size - new_size < sizeof (heap_header_t)+sizeof (heap_footer_t)) {
    // No need, just adjust values
    size += orig_hole_size - new_size;
    new_size = orig_hole_size;
  }

kprintf("Stage 3\n");

  // Page align the block? Create small hole block in front of it
  if (aligned && orig_hole_pos & 0xFFFFF000) {
    Uint32 new_location = orig_hole_pos + 0x1000 - (orig_hole_pos & 0xFFF) - sizeof (heap_header_t);

    heap_header_t *hole_header = (heap_header_t *)orig_hole_pos;
    hole_header->size = 0x1000 - (orig_hole_pos & 0xFFF) - sizeof (heap_header_t);
    hole_header->magic = HEAP_MAGIC;
    hole_header->is_hole = 1;

    heap_footer_t *hole_footer = (heap_footer_t *)( new_location - sizeof (heap_footer_t));
    hole_footer->magic = HEAP_MAGIC;
    hole_footer->header = hole_header;
    orig_hole_pos = new_location;
    orig_hole_size = orig_hole_size - hole_header->size;
  } else {
    // Delete hole from index
    queue_remove (heap->queue, orig_hole);
  }

kprintf ("Stage 3 sub 1\n");

  // Should we create a new hole block after the allocated block? Allocate memory first
  // before messing up the header structures
  queue_item_t *item;
  if (orig_hole_size - new_size > 0) {
    item = (queue_item_t *)kmalloc (sizeof (queue_item_t));
  }

kprintf("Stage 4\n");
  heap_header_t *block_header = (heap_header_t *)orig_hole_pos;
  block_header->magic = HEAP_MAGIC;
  block_header->is_hole = 0;
  block_header->size = new_size;

  heap_footer_t *block_footer = (heap_footer_t *)(orig_hole_pos + sizeof (heap_header_t) + size);
  block_footer->magic = HEAP_MAGIC;
  block_footer->header = block_header;

kprintf("Stage 5\n");
  // Should we create a new hole block after the allocated block?
  if (orig_hole_size - new_size > 0) {

kprintf ("Stage 5.1\n");
    heap_header_t *hole_header = (heap_header_t *)(orig_hole_pos + sizeof (heap_header_t)+ size + sizeof (heap_footer_t));
    hole_header->magic = HEAP_MAGIC;
    hole_header->is_hole = 1;
    hole_header->size = orig_hole_size - new_size;

kprintf ("Stage 5.2\n");
    heap_footer_t *hole_footer = (heap_footer_t *)((Uint32)hole_header + orig_hole_size - new_size - sizeof (heap_footer_t));
    // Is the block still in range, otherwise don't add (@TODO: is this correct?)
    if ((Uint32)hole_footer < heap->end) {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->header = hole_header;
    }
kprintf ("Stage 5.3\n");
    heap_insert_in_ordered_queue (heap->queue, hole_header, item);
kprintf ("Stage 5.4\n");
  }

kprintf("Stage 6\n");
  return (void *)( (Uint32)block_header + sizeof (heap_header_t) );
}


/**
 *
 */
void _heap_free (heap_t *heap, void *p) {
  if (p == 0) return;

  heap_header_t *header = (heap_header_t *)( (Uint32)p - sizeof (heap_header_t) );
  heap_footer_t *footer = (heap_footer_t *)( (Uint32)header + header->size - sizeof (heap_footer_t));

  if (header->magic != HEAP_MAGIC) return;
  if (footer->magic != HEAP_MAGIC) return;

  header->is_hole = 1;
  char do_add = 1;

  // Unify left
  heap_footer_t *test_footer = (heap_footer_t *)( (Uint32)header - sizeof (heap_footer_t));
  if (test_footer->magic == HEAP_MAGIC && test_footer->header->is_hole == 1) {
    // It's another hole, merge them
    Uint32 cache_size = header->size;
    header = test_footer->header;
    footer->header = header;
    header->size += cache_size;
    do_add = 0;
  }

  // Unify right
  heap_header_t *test_header = (heap_header_t *)( (Uint32)footer + sizeof (heap_footer_t));
  if (test_header->magic == HEAP_MAGIC && test_header->is_hole == 1) {
    header->size += test_header->size;
    test_footer = (heap_footer_t *)( (Uint32)test_header + test_header->size - sizeof (heap_footer_t));
    footer = test_footer;

   // Find and remove obsolete header
   _heap_callback_findheader_var = (Uint32)test_header;
   queue_item_t *item = queue_seek (heap->queue, _heap_callback_findheader);
   if (item) queue_remove (heap->queue, item);
  }


  // Is this footer the end of the heap, we can contract the heap
  if ( (Uint32)footer + sizeof (heap_footer_t) == heap->end) {
    Uint32 old_length = heap->end - heap->start;
    Uint32 new_length = heap_contract (heap, (Uint32)header - heap->start);

    // Is this whole block contracted?
    if (header->size - (old_length - new_length) > 0) {
      // No, adjust block
      header->size -= old_length - new_length;
      footer = (heap_footer_t *)( (Uint32)header + header->size - sizeof (heap_footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
    } else {
      // We can remove this block.
      _heap_callback_findheader_var = (Uint32)test_header;
      queue_item_t *item = queue_seek (heap->queue, _heap_callback_findheader);
      if (item) queue_remove (heap->queue, item);
    }
  }

  if (do_add == 1) {
    heap_insert_in_ordered_queue (heap->queue, header, NULL);
  }
}


/**
 *
 */
void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address) {
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
  _heap_free (kheap, ptr);
}


/**
 *
 */
void print_heap_info (heap_t *heap) {
  kprintf ("======== Heap info for heap %08X ==========\n", heap);
  kprintf ("\n");

  kprintf ("Heap start   : %08X\n", heap->start);
  kprintf ("Heap end     : %08X\n", heap->end);
  kprintf ("Heap max     : %08X\n", heap->max);
  kprintf ("Current size : %08X\n", heap->end - heap->start);
  kprintf ("\n");
  kprintf ("\n");

  kprintf ("Queue info (%d items):\n", heap->queue->count);
  queue_item_t *item = NULL;
  while (item = queue_iterate (heap->queue, item), item) {
    kprintf ("item: %08X\n", item->data);
    heap_header_t *header = (heap_header_t *)item->data;
    kprintf ("item->magic  : %08X\n", header->magic);
    kprintf ("item->size   : %08X\n", header->size);
    kprintf ("item->is_hole: %08X\n", header->is_hole);
  }
  kprintf ("\n");
  kprintf ("\n");

  kprintf ("Link info:\n");
  heap_header_t *header = (heap_header_t *)heap->start;
  while ((Uint32)header < heap->end) {
    kprintf ("Block header: %08X\n", header);
    kprintf ("  Magic   : %08X\n", header->magic);
    kprintf ("  Size    : %08X\n", header->size);
    kprintf ("  is_hole : %08X\n", header->is_hole);

    heap_footer_t *footer = (heap_footer_t *)( (Uint32)header + header->size - sizeof (heap_footer_t));
    kprintf ("Block footer: %08X\n", footer);
    kprintf ("  Magic   : %08X\n", footer->magic);
    kprintf ("  header  : %08X\n", footer->header);

    // Next header
    header += header->size + sizeof (heap_header_t) + sizeof (heap_footer_t);
  }

  kprintf ("\n\n");
}