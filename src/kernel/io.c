/******************************************************************************
 *
 *  File        : io.c
 *  Description : Low level input/output functions
 *
 *****************************************************************************/
#include "io.h"

/*******************************************************
 * Outputs a byte trough a memory port.
 *
 * In  : addr = port number
 *       b    = byte to send
 *
 */
void outb (Uint32 addr, Uint8 b) {
  __asm__ __volatile__ ("outb  %%al, %%dx" : : "d" (addr), "a" (b) );
}

/*******************************************************
 * Outputs a word trough a memory port.
 *
 * In  : addr = port number
 *       w    = word to send
 *
 */
void outw (Uint32 addr, Uint16 w) {
  __asm__ __volatile__ ("outw  %%ax, %%dx" : : "d" (addr), "a" (w) );
}

/*******************************************************
 * Outputs a dword trough a memory port.
 *
 * In  : addr = port number
 *       l    = dword to send
 *
 */
void outl (Uint32 addr, Uint32 l) {
  __asm__ __volatile__ ("outl  %%eax, %%dx" : : "d" (addr), "a" (l) );
}

/*******************************************************
 * Gets a byte from a memory port.
 *
 * In  : addr = port number
 *
 * Out : byte
 */
Uint8 inb (Uint32 addr) {
  Uint8 b;

  __asm__ __volatile__ ("inb  %%dx, %%al" : "=a" (b) : "d" (addr) );
  return b;
}

/*******************************************************
 * Gets a word from a memory port.
 *
 * In  : addr = port number
 *
 * Out : word
 */
Uint16 inw (Uint32 addr) {
  Uint16 w;

  __asm__ __volatile__ ("inw  %%dx, %%ax" : "=a" (w) : "d" (addr) );
  return w;
}

/*******************************************************
 * Gets a dword from a memory port.
 *
 * In  : addr = port number
 *
 * Out : dword
 */
Uint32 inl (Uint32 addr) {
  Uint32 l;

  __asm__ __volatile__ ("inl  %%dx, %%eax" : "=a" (l) : "d" (addr) );
  return l;
}


/*******************************************************
 * IO wait function
 */
void io_wait (void) {
  outb (0x80, 0);     // Unused port (only during POST)
}

