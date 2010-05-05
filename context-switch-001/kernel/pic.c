/******************************************************************************
 *
 *  File        : pic.c
 *  Description : Programmable interrupt controler functions.
 *
 *****************************************************************************/
#include "errors.h"
#include "kernel.h"
#include "klib.h"
#include "pic.h"
#include "io.h"
#include "idt.h"


/*****************************************************************************
 * Masks the master and slave IRQ lines. lo word are for the master (0..7) and
 * the hi word are the slave irq's. Since the slave PIC is connected to the
 * master IRQ 2, disabeling IRQ2 on the master will disable ALL irq's of the
 * slave.
 */
int pic_mask_irq (Uint16 irq_mask) {
  outb (0x21, LO8 (irq_mask));
  outb (0x21, HI8 (irq_mask));

  return ERR_OK;
}

/*****************************************************************************
 * Initialises and redirects the IRQ's from 08..0F/70..78 to 50..5F. This way
 * they don't interfere with our protected mode exceptions.
 */
int pic_init (void) {
  outb (0x20, 0x11);    // Output ICW1 to master 8259
  outb (0xA0, 0x11);    // Output ICW1 to slave 8259

  outb (0x21, IRQINT_START);     // ICW2: Master IRQ0..7 on 50..57
  outb (0xA1, IRQINT_START+7);   // ICW2: Slave IRQ8..F on 58..5F

  outb (0x21, 0x04);    // Slave PIC connected to Master IRQ 2
  outb (0xA1, 0x02);    // Slave PIC connected to Master IRQ 2

  outb (0x21, 0x01);    // ICW4 to master pic
  outb (0xA1, 0x01);    // ICW4 to slave pic

  pic_mask_irq (0xffff);

  return ERR_OK;
}
