/******************************************************************************
 *
 *  File        : pic.c
 *  Description : Programmable interrupt controler function headers.
 *
 *****************************************************************************/
#ifndef __PIC_H__
#define __PIC_H__

  #include "ktype.h"

  int pic_init (void);
  int pic_mask_irq (Uint16 irq_mask);

#endif //__PIC_H__
