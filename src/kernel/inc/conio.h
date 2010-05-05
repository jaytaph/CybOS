/******************************************************************************
 *
 *  File        : conio.h
 *  Description : Console (screen) I/O functions
 *
 *****************************************************************************/

#ifndef __CONIO_H__
#define __CONIO_H__

  #include "console.h"

  int con_flush (TCONSOLE *console);

  int con_update_screen (TCONSOLE *console);
  int con_set_palette (TCONSOLE *console);
  int con_set_vga_palette (char *palette);

  int con_clrscr (TCONSOLE *console);
  int con_plot (TCONSOLE *console, int px, int py, char ch);
  int con_directputch (TCONSOLE *console, char ch);
  int con_putch (TCONSOLE *console, char ch);
  int con_printf (TCONSOLE *console, const char *fmt, ...);

  int con_setattr (TCONSOLE *console, int attr);
  int con_getattr (TCONSOLE *console);

  int con_get_vga_cursor (void);
  int con_set_vga_cursor (int cursor_offset);

  int con_wait_vertical_retrace (int retraces);

  int con_setxy (TCONSOLE *console, int x, int y);
  int con_getxy (TCONSOLE *console, int *x, int *y);

  unsigned char con_getch (TCONSOLE *console);
  int con_gets (TCONSOLE *console, char *str, int len);

  int con_stuffkey (TCONSOLE *console, unsigned char key[]);

#endif //__CONSOLE_H__
