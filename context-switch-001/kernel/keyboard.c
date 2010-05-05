/******************************************************************************
 *
 *  File        : keyboard.c
 *  Description : Keyboard handling functions
 *
 *****************************************************************************/
#include "keyboard.h"
#include "console.h"
#include "kernel.h"
#include "errors.h"
#include "conio.h"
#include "keys.h"
#include "io.h"

// Key status flags
static int KEY_ALT      = 0;
static int KEY_CTRL     = 0;
static int KEY_LSHIFT   = 0;
static int KEY_RSHIFT   = 0;
static int KEY_NUMLOCK  = 1;
static int KEY_CAPSLOCK = 0;
static int KEY_SCRLOCK  = 0;


unsigned char keybuf[MAX_KEYBUF];
int keyptr = 0;

// Special <ctrl><tab> switches
static int  in_console_switch  = 0;      // 1 if we are currently ctrl-tabbing
static TCONSOLE *ctrltab_console;        // Points to the console we currently select in the ctrltab-bar
static char ctrltab_bar[1024];           // Console switch line.  TODO: must be allocated dynamically since every console max_px can be of different size.



/************************************************************
 * Returns hexnumber (max 0xFF).
 *
 * In  : nr  = hex number to convert
 *       pos = position (1 or 2) which we want to return
 *
 * Out:  ascii-char of hexnumber's position.
 *
 */
unsigned char hex (int nr, int pos) {
  int i;

  if (pos==1) {
    i=nr % 16;
    if (i>9) return 'A'+(i-10); else return '0'+i;
  } else {
    i=nr / 16;
    if (i>9) return 'A'+(i-10); else return '0'+i;
  }
}

/************************************************************
 * Set the leds on the keyboard.
 *
 * In:    c = Caps lock flag (0 = off, 1 = on)
 *        n = Numm lock flag (0 = off, 1 = on)
 *        s = Scrl lock flag (0 = off, 1 = on)
 */
void keyboard_setleds (int c, int n, int s) {
  int ledstatus= (n*2+s*1+c*4);

  outb (0x60, 0xED);
  while (inb (0x64) & 2);
  outb (0x60, ledstatus);
  while (inb (0x64) & 2);
}

// Note that key[0] is the length of the key-string (which starts at key[1])
int keyboard_stuffkey (unsigned char key[]) {
  int i;

  // Don't add if there isn't enough space in the keybuf
  if (keyptr+key[0] >= MAX_KEYBUF) return ERR_ERROR;

  // Add all valid key-entries
  for (i=0; i!=key[0]; i++) keybuf[keyptr++] = key[i+1];

  // Return
  return ERR_OK;
}


/************************************************************
 * Interrupt Service routine which handles the keyboard on IRQ 1
 */
int keyboard_interrupt (void) {
  unsigned char scancode;
  unsigned char key[10];
  unsigned int make,key_ascii;

  // Get character from the keyboard port
  scancode = inb (0x60);

  // Is it a make or brake code?
  make = ((scancode & 0x80)==0);

  // Plot some debug stuff on the screen
  con_plot (_kconsole, 50, 00, '[');
  con_plot (_kconsole, 51, 00, hex (scancode,0));
  con_plot (_kconsole, 52, 00, hex (scancode,1));
  con_plot (_kconsole, 53, 00, ']');

  // Get the "original" scancode
  scancode &= 0x7F;

  switch (scancode) {
    case 0x1D   : // CTRL pressed
                  if (make) {
                    KEY_CTRL=1;  // Make?
                  } else {
                    KEY_CTRL=0;  // Break?
                    // if we're currently in a <ctrl><tab> switch, we must
                    // stop it since we released the <ctrl>
                    if (ctrltab_console != NULL) console_switch_fini();
                  }
                  break;
    case 0x38   : // ALT pressed
                  if (make) KEY_ALT=1; else KEY_ALT=0;
    case 0x3A   : // CAPSLOCK pressed
                  KEY_CAPSLOCK = (KEY_CAPSLOCK == 0) ? 1 : 0;
                  keyboard_setleds (KEY_CAPSLOCK, KEY_NUMLOCK, KEY_SCRLOCK);
                  break;
    case 0x45   : // NUMLOCK pressed
                  KEY_NUMLOCK = (KEY_NUMLOCK == 0) ? 1 : 0;
                  keyboard_setleds (KEY_CAPSLOCK, KEY_NUMLOCK, KEY_SCRLOCK);
                  break;
    case 0x46   : // SRCLOCK pressed
                  KEY_SCRLOCK = (KEY_SCRLOCK == 0) ? 1 : 0;
                  keyboard_setleds (KEY_CAPSLOCK, KEY_NUMLOCK, KEY_SCRLOCK);
                  break;

        // Dit is mooi klote.. Ik hou Lshift in, ik hou Rshift in,
        // ik laat Rshift los, ik laat Lshift los, krijg ik geen
        // brake-code voor Lshift terug... GRMBL. Zodra als ik dus *UN*
        // shift loslaat, dan zijn ze beide gebroken, ook al hou ik dan die
        // andere nog in.... Hier wor je echt niet goed van... schijtzooi..
    case 0x36   : // Left Shift
                  if (make) KEY_LSHIFT=1; else { KEY_LSHIFT=0; KEY_RSHIFT=0; }
    case 0x2A   : // Right Shift
                  if (make) KEY_RSHIFT=1; else { KEY_LSHIFT=0; KEY_RSHIFT=0; }
                  break;
  }

  // Did we press tab?
  // Yes we did, and did we press (make) the CTRL key?
  // Yes.. we want a console switch.. Initialize the damn thingy!!
  if (scancode==0x0F && make && in_console_switch==0 && KEY_CTRL) console_switch_init ();

  // We are choosing a console,
  if (scancode==0x0F && make && in_console_switch==1) console_switch_run();

  // Do not interpret other keys while we're in a control switch. First we
  // must leave it (by releasing <ctrl>) in order to handle other keys again.
  if (in_console_switch==1) return 0;


  // Nothing special occurs.. now we can convert the
  // scancode+alt+ctrl+shift-status into the correct "ascii"-key. After this we
  // stuff this key into the keyboard buffer (unless the buffer is full).
  if (make) {
    if (KEY_LSHIFT || KEY_RSHIFT) key_ascii = scan2ascii_table[scancode][1];
    else if (KEY_CTRL) key_ascii = scan2ascii_table[scancode][2];
    else if (KEY_ALT) key_ascii = scan2ascii_table[scancode][3];
    else if (KEY_NUMLOCK) key_ascii = scan2ascii_table[scancode][4];
    else if (KEY_NUMLOCK && (KEY_LSHIFT || KEY_RSHIFT))
      key_ascii = scan2ascii_table[scancode][7];
    else if (KEY_CAPSLOCK) key_ascii = scan2ascii_table[scancode][5];
    else if (KEY_CAPSLOCK && (KEY_LSHIFT || KEY_RSHIFT))
      key_ascii = scan2ascii_table[scancode][6];
    else key_ascii = scan2ascii_table[scancode][0];


    // Stuff the key...
    if (key_ascii != 0) {
      if (key_ascii > 0xFF) {
        key[0]=2;
        key[1]=key_ascii & 0x00FF;
        key[2]=key_ascii & 0xFF00;
      } else {
        key[0]=1;
        key[1]=key_ascii;
      }
      keyboard_stuffkey (key);
    }
  }

  // Plot some debug info (again)
  con_plot (_kconsole, 39, 0, '[');
  if (KEY_ALT)      con_plot (_kconsole, 40, 0, 'A'); else con_plot (_kconsole, 40, 0, '_');
  if (KEY_CTRL)     con_plot (_kconsole, 41, 0, 'C'); else con_plot (_kconsole, 41, 0, '_');
  if (KEY_RSHIFT)   con_plot (_kconsole, 42, 0, 'S'); else con_plot (_kconsole, 42, 0, '_');
  if (KEY_LSHIFT)   con_plot (_kconsole, 43, 0, 'S'); else con_plot (_kconsole, 43, 0, '_');
  if (KEY_NUMLOCK)  con_plot (_kconsole, 44, 0, 'N'); else con_plot (_kconsole, 44, 0, '_');
  if (KEY_CAPSLOCK) con_plot (_kconsole, 45, 0, 'C'); else con_plot (_kconsole, 45, 0, '_');
  if (KEY_SCRLOCK)  con_plot (_kconsole, 46, 0, 'S'); else con_plot (_kconsole, 46, 0, '_');
  if (in_console_switch) con_plot (_kconsole, 47, 0, '!'); else con_plot (_kconsole, 47, 0, '_');
  con_plot (_kconsole, 48, 0, ']');
  con_flush (_kconsole);

  // Keyboard never triggers a rescheduling
  return 0;
}


/************************************************************
 * Cleans up everyhing we messed up by pressing <ctrl><tab> :)
 * Function does not return anything since it's called from an interrupt
 * handler!
 */
void console_switch_fini (void) {
  // We are done with the console_switch
  in_console_switch = 0;

  // Switch console, force an update so it automatically stops displaying the ctrltab-bar.
  switch_console (ctrltab_console, 1);

  // return (to the keyboard handler)
}

/************************************************************
 * Initializes a <ctrl><tab> in which we can switch screens (novell way)
 * Function does not return anything since it's called from an interrupt
 * handler!
 */
void console_switch_init (void) {
  // We start with showing the current console (so pressing ctrltab once
  // just displays the current active console). To do this, we fetch
  // the current console and select the previous one. From that point,
  // the first call to console_switch_run() will display the next console,
  // which is the current console.
  ctrltab_console = get_current_console();

  // Start the console_switch by setting the flag. Keyboard handler
  // will do the rest (since this function is called by the keyboard handler)
  in_console_switch = 1;

  // return (to the keyboard handler)
}

/************************************************************
 * Gets the next console in the list and shows it on the bottom of the page.
 * Todo: must change the bar-creation to use the console max_px and max_py.
 *
 * Function does not return anything since it's called from an interrupt
 * handler!
 */
void console_switch_run (void) {
  char *conname;
  int i;

  // Get next console and put in ctrltab_console
  ctrltab_console = get_next_console_in_list (ctrltab_console);

  // Make a blue bar in the console switch line
  for (i=0; i!=160; ) {
    ctrltab_bar[i++] = ' ';
    ctrltab_bar[i++] = 1*16+15;
  }

  // And add the next console name
  conname = ctrltab_console->name;
  for (i=0; i!=strlen (conname); i++) ctrltab_bar[2+(i*2)] = conname[i];

  if (ctrltab_console == get_current_console ()) {
    ctrltab_bar[2+(i*2)+0] = ' ';
    ctrltab_bar[2+(i*2)+2] = '(';
    ctrltab_bar[2+(i*2)+4] = '*';
    ctrltab_bar[2+(i*2)+6] = ')';
  }

  // Update screen to display the ctrltab bar
  con_update_screen (get_current_console());

  // return (to the keyboard handler)
}

/************************************************************
 * Returns 1 if we currently have a ctrltab-bar on the screen
 */
int console_switch_active () {
  return (in_console_switch == 1);
}

/************************************************************
 * Returns the ctrltab bar itself
 */
char *console_get_ctrltab_bar () {
  return &ctrltab_bar[0];
}


/********************************************************************
 * Get a character on from the keyboardhandler on a console
 *
 * In  : con_idx = index number of the console.
 *
 * Out : ascii value char from keybuffer or -1 on error
 */
unsigned char keyboard_poll (void) {
  unsigned char ch;

  // wait until a key is placed in the keybuffer
  // by an "externel" source. This is most likely the keyboard
  // handler on IRQ 1 but could also be done by another process.
  while (keyptr == 0) ;

  // Remember the key we return
  ch = keybuf[0];

  // Because we're moving all characters up in the keyboard buffer, for a split
  // moment the 'keyptr' is out of sync. We disable interrupts for this brief
  // time so the keyboard handler or any other external source doesn't use the
  // unsync'ed pointer.
  cli ();

  // move all keys up in the buffer
  memcpy (keybuf+0, keybuf+1, MAX_KEYBUF-1);

  // decrease pointer so it's synced again...
  if (keyptr != 0) keyptr--;

  // Synced, we can use the buffer
  sti ();

  // Return
  return ch;
}


