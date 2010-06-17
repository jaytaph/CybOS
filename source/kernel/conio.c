/******************************************************************************
 *
 *  File        : conio.c
 *  Description : Low-level console IO Functions for direct manipulation of the
 *                consoles.
 *
 *****************************************************************************/

#include "schedule.h"
#include "console.h"
#include "kernel.h"
#include "kmem.h"
#include "keyboard.h"
#include "errors.h"
#include "conio.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"




/********************************************************************
 * Set cursor at the cursor_offset directory through VGA-ports.
 *
 * In  : cursor_offset    = absolute offset of cursor
 *
 * Out : -1 on error, 1 on OK
 */
int con_set_vga_cursor (int cursor_offset) {
  // Set VGA Cursor through vga io-ports
  outb (0x3d4, 0x0E);
  outb (0x3d5, HI8(cursor_offset));
  outb (0x3d4, 0x0F);
  outb (0x3d5, LO8(cursor_offset));

  return ERR_OK;
}

/********************************************************************
 * Set cursor at the cursor_offset directory through VGA-ports.
 *
 * In  : cursor_offset    = absolute offset of cursor
 *
 * Out : -1 on error, 1 on OK
 */
int con_get_vga_cursor (void) {
  int lo,hi;

  // Set VGA Cursor through vga io-ports
  outb (0x3d4, 0x0E);
  hi = inb (0x3d5);
  outb (0x3d4, 0x0F);
  lo = inb (0x3d5);

  return (hi * 256 + lo);
}


/********************************************************************
 * Clears the screenbuffer and updates the screen (if necessary)
 *
 * In  : con_idx = index number of the console.
 *
 * Out : -1 on error, 1 on OK
 */
int con_clrscr (console_t *console) {
  int i;

  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Clear the buffer.
  i=0;
  while (i != console->size) {
    console->buf[i++] = ' ';
    console->buf[i++] = console->attr;
  }

  // Goto start of the screen and flush screen
  con_setxy (console, 0, 0);
  con_flush (console);

  // Return
  return ERR_OK;
}


/********************************************************************
 * Scrolls the buffer up 1 line and displays is on the screen if
 * necessary.
 *
 * In  : con_idx = index number of the console.
 *
 * Out : -1 on error, 1 on OK
 */
int con_scrollup (console_t *console) {
  int size, start;

  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Calculate size of the move (note: we don't move the first line)
  size = console->size - (console->max_px*2);

  // Starting offset of the move
  start = console->max_px * 2;

  // Move from starting offset to 0 in the buffer.
  memmove (console->buf+0, console->buf+start, size);

  // Empty the line which is now double on screen
  memset (console->buf+size, 0, start);

  // Return
  return ERR_OK;
}


/********************************************************************
 * Plots a character directly to a buffer without advancing the
 * cursor. If printed on the current screen, the characted will be
 * automaticly be displayed on the VGA All ascii-chars will be printed.
 *
 * In  : con_idx = index number of the console.
 *       x       = x position (0 based)
 *       y       = y position (0 based)
 *       ch      = character to print
 *
 * Out : -1 on error, 1 on OK
 */
int con_plot (console_t *console, int x, int y, char ch) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  int offset = (y * console->max_px + x) * 2;

  console->buf[offset+0] = ch;             // Write character
  console->buf[offset+1] = console->attr;  // in the default attribute

  return ERR_OK;
}


/********************************************************************
 * Plots a DIRECT character on a console and updates the cursor
 * position. This function can also print specials chars (lower than
 * 31) directly on the screen without interpreting them (like \n, \b etc)
 *
 * In  : con_idx = index number of the console.
 *       ch      = character to print
 *
 * Out : -1 on error, 1 on OK
 */
int con_directputch (console_t *console, char ch) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Plots the char directly to the screen
  con_plot (console, console->px, console->py, ch);

  // Move cursor to the next position
  console->px++;

  // Check the X Y positions
  if (console->px == console->max_px) {
    console->px = 0;

    // If we are on the bottom, don't increase PY
    if (console->py == console->max_py-1) {
      con_scrollup (console);
    } else {
      console->py++;
    }
  }

  // Return
  return ERR_OK;
}


/********************************************************************
 * Plots a character on a console and updates the cursor position.
 * Some chars below 32 will be threaded as a special character.
 *
 * In  : con_idx = index number of the console.
 *       ch      = character to print
 *
 * Out : -1 on error, 1 on OK
 */
int con_putch (console_t *console, char ch) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // New Line
  if (ch == '\n') {
    console->px = 0;

    // If we are on the bottom, don't increase PY
    if (console->py == console->max_py - 1) {
      con_scrollup (console);
    } else {
      console->py++;
    }
    return ERR_OK;
  }

  // Carriage Return
  if (ch == '\r') {
    console->px=0;

    return ERR_OK;
  }

  // Backspace
  if (ch == '\b') {
    // No backspace when we're on top of the screen
    if (console->px == 0 && console->py == 0) {
      return ERR_OK;
    }

    // X = 0, then remove char on the previous line
    if (console->px == 0) {
      console->py--;
      console->px = console->max_px;
      con_plot (console, console->px, console->py, ' ');

      return ERR_OK;
    }

    // Nothing special with the backspace
    console->px--;
    con_plot (console, console->px, console->py, ' ');

    return ERR_OK;
  }

  // Normal character. Print it and let con_directputch take care of our error level
  return con_directputch (console, ch);
}


/********************************************************************
 * Flushes the console buffer to the screen but only when the
 * "con_idx" is physicaly on the screen. Doesn't do anything when it's
 * not.
 *
 * In  : con_idx = index number of the console.
 *
 * Out : -1 on error, 1 on OK
 */
int con_flush (console_t *console) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Update the screen, only when the console is currently on the screen.
  if (get_current_console() != console) return ERR_OK;

  // Update screen
  con_update_screen (console);

  // Set cursor position
  con_set_vga_cursor (console->py * console->max_px + console->px);
  console->opy = console->py;
  console->opx = console->px;

  // Return
  return ERR_OK;
}


/********************************************************************
 * Low level function to copy the buffer of console "con_idx" to the
 * screen in selector 0x08 (must change hardcoded stuff into defines)
 *
 * In  : con_idx = index number of the console.
 *
 * Out : -1 on error, 1 on OK
 */
int con_update_screen (console_t *console) {
  char *ctrltab_bar;

  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Wait for vertical retrace before continuing
  con_wait_vertical_retrace (1);

  if (console_switch_active ()) {
    // Display the blue bar at the end of the screen?
    ctrltab_bar = console_get_ctrltab_bar ();

    // Copy buffer to the screen, except for the last line and add ctrltab-bar there
    memmove ((char *)0xF00B8000, console->buf, console->size-(console->max_px*2));
    memmove ((char *)0xF00B8000+((console->max_py-1)*console->max_px*2), ctrltab_bar, (console->max_px*2));

  } else {
    // Just copy the whole buffer to the screen
    memmove ((char *)0xF00B8000, console->buf, console->size);
  }

  return ERR_OK;
}


/********************************************************************
 * Writes the palette of the console to the VGA DAC. Again: not
 * video-card safe manners.. :-(
 *
 * In  : con_idx = index number of the console.
 *
 * Out : -1 on error, 1 on OK
 */
int con_set_palette (console_t *console) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  con_wait_vertical_retrace (1);
  con_set_vga_palette (console->palette);

  return ERR_OK;
}

int con_set_vga_palette (char *palette) {
  int i;

  // Output base address to start and start stuffing rgb values to the DAC.
  outb (0x3C8, 0);
  for (i=0; i!=768; i++) outb (0x3C9, palette[i]);

  return ERR_OK;
}


/********************************************************************
 * Waits until the end of a vertical retrace.
 *
 * In  : retraces = Number of retraces to wait
 *
 * Out : -1 on error, 1 on OK
 */
int con_wait_vertical_retrace (int retraces) {
  int i;

  // Repeat until ray cannon returns from the lower right corner
  // back to the upper left corner (vertical retrace)
  for (i=0; i!=retraces; i++) {
    while ((inb (0x3DA)&8)==8) ;
    while ((inb (0x3DA)&8)!=8) ;
  }

  // Return
  return ERR_OK;
}


/********************************************************************
 * Plots the buffer of the current console on the screen.
 *
 * In  : ch     = character to print to the screen
 *       **ptr  = nasty pointer. holds the con_idx value instead of
 *                pointer to an address that holds the con_idx value.
 *
 * Out : always 0
 */
int con_printf_helper (char ch, void **ptr) {
  con_putch ((console_t *)ptr, ch);
  return 1;
}


/********************************************************************
 * Prints the string to the con_idx's console buffer. Takes care of
 * updating the screen when it's the current console.
 *
 * In  : con_idx = index number of the console.
 *       fmt     = string which holds a normal "printf" format
 *       ...     = unknown number of parameters of all kinds (va_list)
 *
 * Out : -1 on error, 1 on OK
 */
int con_printf (console_t *console, const char *fmt, ...) {
  va_list args;

  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  va_start (args, fmt);
  do_printf (fmt, args, &con_printf_helper, (void *)console);
  va_end (args);

  // Show the string we just printed if necessary...
  con_flush (console);

  // Return
  return ERR_OK;
}


/********************************************************************
 * Places cursor at console "con_idx" on position <X,Y>
 *
 * In  : con_idx = index number of the console.
 *       x       = X position (0 based)
 *       y       = Y position (0 based)
 *
 * Out : -1 on error, 1 on OK
 */
int con_setxy (console_t *console, int x, int y) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Make sure our values aren't bigger than the console boundaries.
  if (x>=console->max_px || y>=console->max_py) return ERR_ERROR;

  // Set position
  console->px=x;
  console->py=y;

  // And correct the cursor on the screen (if necessary)
  if (get_current_console() == console) {
    con_set_vga_cursor ((console->py * console->max_px) + console->px);
  }

  // Return
  return ERR_OK;
}


/********************************************************************
 * Get cursor position of a console.
 *
 * In  : con_idx = index number of the console.
 *       *x      = storage for x position
 *       *y      = storage for y position
 *
 * Out : -1 on error, 1 on OK
 */
int con_getxy (console_t *console, int *x, int *y) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Get position
  *x = console->px;
  *y = console->py;

  // Return
  return ERR_OK;
}


/********************************************************************
 * Set attribute
 *
 * In  : con_idx = index number of the console.
 *       attr    = attribute (background * 16 + foreground)
 *
 * Out : -1 on error, 1 on OK
 */
int con_setattr (console_t *console, int attr) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Set current attribute
  console->attr = attr;

  // Return
  return ERR_OK;
}


/********************************************************************
 * Get attribute
 *
 * In  : con_idx = index number of the console.
 *
 * Out : Attribute of the console or -1 on error
 */
int con_getattr (console_t *console) {
  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  // Return attribute
  return console->attr;
}


/********************************************************************
 * Gets a string of maximum 'len' chars.
 * ToDo:
 *  - use arrow keys which holds history (char *history[]).
 *  - 'str' can hold a default string which will be printed on the
 *    screen.
 *  - decent edit function instead of just backspace.
 *  - timeout (?)
 *
 * In  : con_idx = index number of the console.
 *       *str    = enough room to store the string
 *       len     = maximum length of the string
 *
 * Out : -1 on error, 1 on OK
 */
int con_gets (console_t *console, char *str, int len) {
  unsigned char ch;
  int tmplen=0;

  if (console == NULL) return ERR_CON_INVALID_CONSOLE;

  do {
    // Get a key from the console
    ch = keyboard_poll ();

    switch (ch) {
      // Backspace
      case 8  : // Remove last char by placing a #0 on top of it
                if (tmplen!=0) {
                  tmplen--;
                  str[tmplen]='\0';
                  con_putch (console, 8);         // Backspace
                  con_flush (console);
                }
                break;
      // Enter
      case 13 :
                break;
      default :
                if (tmplen!=len) {       // Only accept char when we're not at the maximum length
                  str[tmplen]=ch;
                  str[tmplen+1]='\0';
                  con_putch (console, ch);
                  con_flush (console);
                  tmplen++;
                }
                break;
    }
  } while (ch!=13); // repeat until <enter> pressed.

  return ERR_OK;
}


