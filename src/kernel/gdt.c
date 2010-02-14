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
unsigned long long *_kernel_gdt;



// =================================================================================
unsigned long long tss_set (int index, unsigned long long tss) {
  return _kernel_gdt[index] = tss;
}


// =================================================================================
unsigned long long ldt_set (int index, unsigned long long ldt) {
	return _kernel_gdt[index] = ldt;
}


// =================================================================================
unsigned long long gdt_set (int index, unsigned long long gdt) {
	return _kernel_gdt[index] = gdt;
}


// =================================================================================
unsigned long long idt_set (int index, unsigned long long idt) {
	return _kernel_idt[index] = idt;
}


// =================================================================================
unsigned long long tss_get (int index) {
	return _kernel_gdt[index];
}


// =================================================================================
unsigned long long ldt_get (int index) {
	return _kernel_gdt[index];
}


// =================================================================================
unsigned long long gdt_get (int index) {
	return _kernel_gdt[index];
}


// =================================================================================
unsigned long long idt_get (int index) {
	return _kernel_idt[index];
}


// =================================================================================
// Creates a GDT descriptor (either a normal gdt code/data, a TSS or a LDT descriptor)
unsigned long long gdt_create_descriptor (Uint32 base, Uint32 limit, Uint8 flags1, Uint8 flags2) {
  unsigned long long descriptor = 0;

  descriptor |= (unsigned long long)limit;
  descriptor |= (unsigned long long)LO16(base)<<16;
  descriptor |= (unsigned long long)LO8(HI16(base))<<32;
  descriptor |= (unsigned long long)flags1<<40;

  // The bits in Flags2 are already shifted (granularity = 0x80 instead of 0x8)
  // We mask both sides of the byte off since these 2 items share the same byte
  descriptor |= (unsigned long long)(LO8(HI16(limit))&0x0F)<<48;
  descriptor |= (unsigned long long)((flags2)&0x0F)<<48;

  descriptor |= (unsigned long long)HI8(HI16(base))<<56;
  return descriptor;
}


// =================================================================================
// Creates a IDT descriptor.
unsigned long long idt_create_descriptor (Uint32 offset, Uint16 selector, Uint8 flags) {
  unsigned long long descriptor = 0;

  descriptor |= (unsigned long long)LO16(offset);
  descriptor |= (unsigned long long)HI16(offset) << 48;
  descriptor |= (unsigned long long)selector << 16;
  descriptor |= (unsigned long long)flags << 40;
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

