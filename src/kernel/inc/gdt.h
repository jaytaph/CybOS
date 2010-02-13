/******************************************************************************
 *
 *  File        : gdt.h
 *  Description : Descriptors for the global descriptor table. Use these
 *                defines instead of hardcoded selectors. Changed the defines
 *                into variables so we can select our selectors realtime so
 *                the VGA-screen DOESN'T HAVE to be at index 0x08 anymore.
 *
 *****************************************************************************/
#ifndef __GDT_H__
#define __GDT_H__

  // Constants for creating selectors with SEL()
  #define TI_GDT                      0x00          // Selector points to a descriptor in the GDT
  #define TI_LDT                      0x04          // Selector points to a descriptor in the LDT

  #define RPL_RING0                   0x00          // This selector runs on Ring 0 (KERNEL CODE)
  #define RPL_RING1                   0x01          // This selector runs on Ring 1 (not used)
  #define RPL_RING2                   0x02          // This selector runs on Ring 2 (not used)
  #define RPL_RING3                   0x03          // This selector runs on Ring 3 (USER CODE)


  // Flags for IDT descriptors
  #define IDT_NOT_PRESENT    0x00
  #define IDT_PRESENT        0x80

  #define IDT_DPL0           0x00
  #define IDT_DPL1           0x20
  #define IDT_DPL2           0x40
  #define IDT_DPL3           0x60

  #define IDT_TASK_GATE      0x05
  #define IDT_INTERRUPT_GATE 0x0E
  #define IDT_TRAP_GATE      0x0F


  // Flags1 constants for creating TSS descriptors
  #define GDT_NOT_PRESENT       0x00
  #define GDT_PRESENT           0x80

  #define GDT_DPL_RING0         0x00
  #define GDT_DPL_RING1         0x20
  #define GDT_DPL_RING2         0x40
  #define GDT_DPL_RING3         0x60

  #define GDT_NOT_BUSY          0x00
  #define GDT_BUSY              0x02

  // Flags2 constants for creating TSS descriptors
  #define GDT_NO_GRANULARITY    0x00
  #define GDT_GRANULARITY       0x80

  #define GDT_NOT_AVAILABLE     0x00
  #define GDT_AVAILABLE         0x10

  #define GDT_TYPE_TSS          0x09
  #define GDT_TYPE_DATA         0x10
  #define GDT_TYPE_CODE         0x18
  #define GDT_TYPE_LDT          0x02


  void gdt_init (void);

  // Functions to set and get descriptors in the IDT/GDT
  unsigned long long tss_set (int index, unsigned long long tss);
  unsigned long long tss_get (int index);

  unsigned long long ldt_set (int index, unsigned long long ldt);
  unsigned long long ldt_get (int index);

  unsigned long long gdt_set (int index, unsigned long long gdt);
  unsigned long long gdt_get (int index);

  unsigned long long idt_set (int index, unsigned long long idt);
  unsigned long long idt_get (int index);

  // Create a descriptor in the GDT (could be a normal GDT descriptor but also a TSS or LDT descriptor!)
  unsigned long long gdt_create_descriptor (Uint32 base, Uint32 limit, Uint8 flags1, Uint8 flags2);

  // Creates a IDT descriptor
  unsigned long long idt_create_descriptor (Uint32 offset, Uint16 selector, Uint8 flags);

#endif //__GDT_H__
