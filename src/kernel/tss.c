/******************************************************************************
 *
 *  File        : tss.c
 *  Description : TSS routines and managment.
 *
 *****************************************************************************/

#include "kernel.h"

/*******************************************************
 * Marks or unmarks the TSS-descriptor BUSY flag.
 */
void tss_mark_busy (int descriptor, int busy) {
  unsigned char *b;

  b = (unsigned char *)_kernel_gdt + (descriptor*sizeof (unsigned long long)) + 5;
  if (busy) *b |= 0x02; else *b &= 0xFD;
}

