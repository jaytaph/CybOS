/******************************************************************************
 *
 *  File        : console.c
 *  Description : Low level console managment functions
 *
 *****************************************************************************/

#include "kernel.h"
#include "console.h"
#include "errors.h"
#include "kernel.h"
#include "service.h"
#include "conio.h"
#include "kmem.h"
#include "io.h"

  static char _con_default_palette[768];                    // Default palette for a console initialisation
  console_t *_current_console = NULL;              // Current console on the screen
  console_t *_console_list = NULL;                 // Console linked list
  console_t *_kconsole;                            // Console for kernel info (first screen)

//  // Console defines
//  #define CON_INACTIVE    0
//  #define CON_ACTIVE      1

  #define CON_INVISIBLE   0
  #define CON_VISIBLE     1

/********************************************************************
 * Initializes the console-entry-table and create the construct.
 *
 * Out : -1 on error, 1 on OK
 */
int console_init (void) {
  int i;

  // Get global palette, which is used to create the palettes for the other consoles as default
  outb (0x3C7, 0);
  for (i=0; i!=768; i++) _con_default_palette[i] = inb (0x3C9);

  // Create a construct. We assume that that works...
  if ( (_kconsole = (console_t *)sys_console (SYS_CONSOLE_CREATE, 0, "Construct")) == NULL) {
    // Cannot create console, print '!' and deadlock
    unsigned char *vga = (unsigned char *)0xF00B8000;
    *vga = '!';
    for (;;) { }
  }

  // We have a construct, but its screenbuffer is empty. We copy the current output of
  // the screen (BIOS, Bootloader stuff), and set the cursor, it just looks much nicer i think.

  // Fetch data from videoscreen and place it back into the construct-buffer
  memcpy (_kconsole->buf, (char *)0xF00B8000, _kconsole->size);

  // Get and set cursor
  i = con_get_vga_cursor ();
  con_setxy (_kconsole, i%80, i/80);

  // Switch to the construct.
  switch_console (_kconsole, 1);

  return ERR_OK;
}


int destroy_console (console_t *console) {
  console_t *p,*n;

  kprintf ("destroy_console() not implemented fully yet. Memory not released.");

  p = console->prev;
  n = console->next;
  p->next = n;
  n->prev = p;

  return ERR_OK;
}




// ========================================================
int sys_console (int subcommand, console_t *console, char *name) {
  if (subcommand == SYS_CONSOLE_CREATE) {
    return (int) create_console (name, CON_VISIBLE);

  } else if (subcommand == SYS_CONSOLE_DESTROY) {
    return destroy_console (console);

  }

  return ERR_ERROR;
}


/********************************************************************
 * Creates and initializes a console entry in the table.
 *
 * @odo:
 *   - Add video mode for independent video modes trough the consoles
 *   - Add max X and max Y coordinates for independent screen sizes.
 *
 * In  : name    = name of the console
 *
 * Out : console entry or -1 on error
 */
console_t *create_console (char *name, int visible) {
  console_t *console, *last_console;
  int i;

  // Allocate memory for new console
  console = (console_t *)kmalloc (sizeof (console_t));

  // Fill in all attributes for the console
  strncpy (console->name, name, 20);
  console->name[20] = '\0';

  console->opx    = 1;  // Old cursor positions (must be different than
  console->opy    = 1;  // px and py otherwise cursor won't be updated
                        // until the first new cursor position change)
  console->px     = 0;  // Cursor positions
  console->py     = 0;

  Uint8 *b;

  b = (Uint8 *)0xF000044A;  console->max_px = *b;        // Retreive console sizes from BIOS
  b = (Uint8 *)0xF0000484;  console->max_py = (*b) + 1;
  console->attr   = 0x07;                                // Standard attribute

  // Calculate size of the buffer.
  // (note: times two because of the char-attr thingy)
  console->size = console->max_py * console->max_px * 2;

  // Allocate a console buffer in kernel memory
  console->buf = (char *)kmalloc (console->size);

  // And clear it
  i = 0;
  while (i < console->size) {
    console->buf[i++] = ' ';
    console->buf[i++] = console->attr;
  }

  // Set default palette for this screen
  console->palette = (char *)kmalloc (768);
  memcpy (console->palette, _con_default_palette, 768);


  // First console init?
  if (_console_list == NULL) {
    _console_list = console;   // This is the first console in the list
    console->next = NULL;
    console->prev = NULL;
    console->index = 0;

  } else {
    // Otherwise, find last console, and set new console data
    last_console = _console_list;
    while (last_console->next != NULL) last_console = last_console->next;

    last_console->next = console;
    console->next = NULL;
    console->prev = last_console;
    console->index = last_console->index + 1;
  }

  return console;
}





/********************************************************************
 * Returns the console index which is currently shown on the screen
 *
 * Out : index number of the console
 */
console_t *get_current_console (void) {
  return _current_console;
}

int get_current_console_index (void) {
  return _current_console->index;
}


/********************************************************************
 * Returns wether or not the console index is active.
 *
 * In  : con_idx = index number of the console.
 *
 * Out : ERR_CON_ACTIVE or ERR_CON_INACTIVE
 */
/*
int console_active (int con_idx) {
  if (_con_table[con_idx].active==CON_ACTIVE) {
    return ERR_CON_ACTIVE;
  } else {
    return ERR_CON_INACTIVE;
  }
}
*/

/********************************************************************
 * Shows console buffer of console "con_idx" on the screen if it's
 * not already on the screen.
 *
 * In  : con_idx = index number of the console.
 *       force_update = 1 = always update, even when we are on the current console
 *
 * Out : result code
 */
int switch_console (console_t *new_console, int force_update) {

  // Check if a switch is really needed (ie, if the
  // console is already on the screen)
  if (force_update || _current_console != new_console) {
    _current_console = new_console;
    con_update_screen (new_console);
    con_set_palette (new_console);
    con_setxy (new_console, new_console->px, new_console->py);
  }

  return ERR_OK;
}


/**
 * Returns next console in linked list
 */
console_t *get_next_console_in_list (console_t *console) {
  return console->next != NULL ? console->next : _console_list;
}


/**
 * Returns previous console in linked list
 */
console_t *get_prev_console_in_list (console_t *console) {
  if (console->prev != NULL) return console->prev;

  // We are at the start, so look for the end, and return that console
  while (console->next != NULL) console = console->next;
  return console;
}
