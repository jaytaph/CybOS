/******************************************************************************
 *
 *  File        : keyboard.h
 *  Description : Keyboard handling function defines
 *
 *****************************************************************************/
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

  void keyboard_interrupt (void);

  void keyboard_setleds (int capslock, int numlock, int scrolllock);

  void console_switch_init ();
  void console_switch_fini ();
  void console_switch_run ();

  int console_switch_active ();
  char *console_get_ctrltab_bar ();

  unsigned char keyboard_poll (void);

#endif //__KEYBOARD_H__
