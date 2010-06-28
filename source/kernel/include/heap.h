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

  #define HEAP_MAGIC          0xC4F3B4B3      // Magic number to identify header/footer block

  typedef struct {
    Uint32 magic;
    Uint8 is_hole;
    Uint32 size;
  } heap_header_t;

  typedef struct {
    Uint32 magic;
    heap_header_t *header;
  } heap_footer_t;

  typedef struct {
    queue_t *queue;
    Uint32 start;
    Uint32 end;
    Uint32 max;
    Uint8 supervisor;     // TODO: why not 1 byte with pageflags?
    Uint8 readonly;
  } heap_t;




  int heap_init ();
  void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address);
  void _heap_kfree (void *ptr);

#endif // __HEAP_H__
