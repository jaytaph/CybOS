/******************************************************************************
 *
 *  File        : console.h
 *  Description : Low level console managment function headers and defines
 *
 *****************************************************************************/
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

  #include "ktype.h"


  #define CON_KERNEL_IDX                  0     // First entry (#0) is always the kernel
  #define MAX_KEYBUF                    256     // Maximum number of keys in the keybuffer

  // Console entry
  #pragma pack(1)                 // TODO: Pack it, it doesn't like boundaries anymore somehow (why is that?)
  typedef struct {
    int  index;                   // Console index
    void *next;                   // Pointer to the next console entry
    void *prev;                   // Pointer to the previous console entry

    char name[21];       // 20 chars (and terminating zero)

    Uint8  screenmode;            // Screenmode to be set by bios
    char *palette;                // Current palette for this console

    Uint8  attr;                  // Current attribute
    Uint8  opx,opy;               // Last known X,Y position
    Uint8  px,py;                 // Current X,Y position
    Uint8  max_px,max_py;         // Maximum screen position
    Uint32 size;                  // Size of the buffer (char+attr)

    char *buf;         // Pointer to the screen buffer
  } console_t;


  extern console_t *_console_list;                      // Start of linkedlist to consoles
  extern console_t *_current_console;               // Current console on screen

  // Functions
  int console_init (void);

  console_t *create_console (char *name, int visible);
  int destroy_console (console_t *console);
  unsigned char *get_console_name (console_t *console);

  console_t *get_current_console (void);
  int get_current_console_index (void);

  int switch_console (console_t *console, int force_update);
  console_t *get_next_console_in_list (console_t *console);
  console_t *get_prev_console_in_list (console_t *console);

#endif //__CONSOLE_H__
