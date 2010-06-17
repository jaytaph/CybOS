/******************************************************************************
 *
 *  File        : gdt.c
 *  Description : Global Descriptor Table functions
 *
 *
 *****************************************************************************/
#include "kernel.h"
#include "kmem.h"

// Structure for GDTR.
#pragma pack(1)
typedef struct { Uint16 limit; Uint32 base; } TGDTR;

// Pointer to the kernel GDT
Uint64 *_kernel_gdt;



// =================================================================================
void gdt_set_descriptor (int index, Uint64 descriptor) {
  _kernel_gdt[index] = descriptor;
}

// =================================================================================
Uint64 gdt_get_descriptor (int index) {
	return _kernel_gdt[index];
}

// =================================================================================
Uint32 gdt_get_base (Uint64 descriptor) {
  Uint64 tmp = descriptor;
  Uint32 base = ((tmp>>48) & 0x0000FFFF) << 16;
  tmp = descriptor;
  base |= (tmp>>16) & 0x0000FFFFFF;
  return base;
}

// =================================================================================
Uint32 gdt_get_limit (Uint64 descriptor) {
  Uint64 tmp = descriptor;
  Uint32 limit = ((tmp >> 48) & 0x000000FF) << 16;
  tmp = descriptor;
  limit |= (tmp & 0x0000FFFF);
  return limit;
}

// =================================================================================
Uint16 gdt_get_flags (Uint64 descriptor) {
  return (Uint16)(descriptor >> 40);
}


// =================================================================================
// Creates a GDT descriptor (either a normal gdt code/data, a TSS or a LDT descriptor)
Uint64 gdt_create_descriptor (Uint32 base, Uint32 limit, Uint8 flags1, Uint8 flags2) {
  Uint64 descriptor = 0;

  descriptor |= (Uint64)limit;
  descriptor |= (Uint64)LO16(base)<<16;
  descriptor |= (Uint64)LO8(HI16(base))<<32;
  descriptor |= (Uint64)flags1<<40;

  // The bits in Flags2 are already shifted (granularity = 0x80 instead of 0x8)
  // We mask both sides of the byte off since these 2 items share the same byte
  descriptor |= (Uint64)(LO8(HI16(limit))&0x0F)<<48;
  descriptor |= (Uint64)((flags2)&0x0F)<<48;

  descriptor |= (Uint64)HI8(HI16(base))<<56;
  return descriptor;
}


// =================================================================================
// Creates a IDT descriptor.
Uint64 idt_create_descriptor (Uint32 offset, Uint16 selector, Uint8 flags) {
  Uint64 descriptor = 0;

  descriptor |= (Uint64)LO16(offset);
  descriptor |= (Uint64)HI16(offset) << 48;
  descriptor |= (Uint64)selector << 16;
  descriptor |= (Uint64)flags << 40;
  return descriptor;
}




// =================================================================================
// Moves the current GDT that is created in the bootloader code into a dynamic
// allocated GDT. This way we can free up the bootloader code and data since nothing
// from the bootsector is used anymore.
void gdt_init (void) {
  TGDTR gdtr;
  Uint32 phys_gdt_addr;


  // Fetch the current GDT
  __asm__ __volatile__ ("sgdt %0" : "=m" (gdtr) : );

  // Allocate room for kernel GDT and save it's physical addres
  _kernel_gdt = kmalloc_physical (gdtr.limit, &phys_gdt_addr);

   // Copy the current GDT data to the new GDT
   memcpy (_kernel_gdt, (void *)gdtr.base, gdtr.limit);

   // Set the correct base and load this GDT
   gdtr.base = phys_gdt_addr;
   gdtr.base += 0xC0000000;     // Need to adjust. The GDT must be available from virtual memory, not physical memory
   __asm__ __volatile__ ("lgdt %0" : "=m" (gdtr) : );
}

