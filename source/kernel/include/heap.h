/******************************************************************************
 *
 *  File        : kmem.h
 *  Description : Kernel Memory Manager
 *
 *****************************************************************************/
#ifndef __HEAP_H__
#define __HEAP_H__

  #include "ktype.h"
  #include "queue.h"

  #define HEAP_START          0xD0000000      // Start of the heap
  #define MIN_HEAP_SIZE       0x50000         // Initial and also minimal heap size
  #define MAX_HEAP_SIZE       0xDFFFF000      // Maximum heap size
  #define HEAP_EXTEND         0x10000         // Grow everytime with this size

  #define HEAP_HEADER_MAGIC   0xC4F3B4B3      // Magic number to identify header block
  #define HEAP_FOOTER_MAGIC   0xB4B3C4F3      // Magic number to identify footer block

  typedef struct {
    Uint32       magic;       // Magic header signature
    Uint8        is_hole;     // 0= data block, 1= hole block
    Uint32       size;        // Size of the block (included header and footer)
    queue_item_t queue_item;  // Queue item for ordered queue
  } heap_header_t;

  typedef struct {
    Uint32        magic;      // Magic footer signature
    heap_header_t *header;    // Pointer to header structure
  } heap_footer_t;

  typedef struct {
    queue_t  *queue;         // Queue with ordered headers for only HOLE blocks (not data)
    Uint32   start;          // Start address of the heap
    Uint32   end;            // End address of the heap
    Uint32   max;            // Maximum grow size
    Uint32   extend;         // Extend size
    Uint8    supervisor;     // 1: supervisor heap (vmm pages with supervisor rights) 0: user
    Uint8    readonly;       // 1: read only heap 0: read/write heap
  } heap_t;


  int heap_init ();

  void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address);
  void _heap_kfree (void *ptr);

#endif // __HEAP_H__
