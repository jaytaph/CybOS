/******************************************************************************
 *
 *  File        : conio.h
 *  Description : Console (screen) I/O functions
 *
 *****************************************************************************/

#ifndef __CONIO_H__
#define __CONIO_H__

  #include "console.h"

  int con_flush (console_t *console);

  int con_update_screen (console_t *console);
  int con_set_palette (console_t *console);
  int con_set_vga_palette (char *palette);

  int con_clrscr (console_t *console);
  int con_plot (console_t *console, int px, int py, char ch);
  int con_directputch (console_t *console, char ch);
  int con_putch (console_t *console, char ch);
  int con_printf (console_t *console, const char *fmt, ...);

  int con_setattr (console_t *console, int attr);
  int con_getattr (console_t *console);

  int con_get_vga_cursor (void);
  int con_set_vga_cursor (int cursor_offset);

  int con_wait_vertical_retrace (int retraces);

  int con_setxy (console_t *console, int x, int y);
  int con_getxy (console_t *console, int *x, int *y);

  unsigned char con_getch (console_t *console);
  int con_gets (console_t *console, char *str, int len);

  int con_stuffkey (console_t *console, unsigned char key[]);

#endif //__CONSOLE_H__
