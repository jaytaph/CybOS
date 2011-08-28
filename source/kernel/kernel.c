/******************************************************************************
 *
 *  File        : kernel.c
 *  Description : Kernel main source file.
 *
 *****************************************************************************/

#include "command.h"
#include "console.h"
#include "version.h"
#include "kernel.h"
#include "klib.h"
#include "paging.h"
#include "ktype.h"
#include "schedule.h"
#include "conio.h"
#include "timer.h"
#include "kmem.h"
#include "heap.h"
#include "service.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "pic.h"
#include "io.h"
#include "dma.h"
#include "queue.h"
#include "pci.h"
#include "exec.h"
#include "drivers/floppy.h"
#include "drivers/ide.h"
#include "vfs.h"
#include "vfs/fat12.h"
#include "vfs/cybfs.h"
#include "vfs/devfs.h"

  // 64bit tick counter
  Uint64 _kernel_ticks;

  // When set to 1, the kernel console automatically flushes after each character plotted.
  Uint8 _kflush = 1;

  // The tasklist of all cybos processes/tasks
  extern task_t *_task_list;

  // Forward defines
  void tprintf (const char *fmt, ...);
  int get_boot_parameter (const char *boot_parameters, const char *arg, char *buffer);





/**
 *
 */
void readdir (vfs_node_t *root, int depth) {
//  kprintf ("readdir ()\n");
  vfs_dirent_t *unsafe_dirent;
  vfs_node_t *unsafe_node;
  vfs_dirent_t local_dirent;
  vfs_node_t local_node;
  int index = 0;
  int j;

  // Never trust the vfs_node_t pointers!
  vfs_node_t local_root_node;
  memcpy (&local_root_node, root, sizeof (vfs_node_t));
//kprintf ("readdir(): node copy\n");

  while (unsafe_dirent = vfs_readdir (&local_root_node, index), unsafe_dirent != NULL) {
    index++;
//    kprintf ("readdir(): Reading index %d\n", index);

    // Copy to local scope
    memcpy (&local_dirent, unsafe_dirent, sizeof (vfs_dirent_t));

    // File cannot be found (huh?)
    unsafe_node = vfs_finddir (&local_root_node, local_dirent.name);
    if (! unsafe_node) continue;

    // Copy to local scope
    memcpy (&local_node, unsafe_node, sizeof (vfs_node_t));

    for (j=0; j!=depth; j++) kprintf ("  ");
    if ((local_node.flags & FS_DIRECTORY) == FS_DIRECTORY)  {
      // This is a directory
      kprintf ("<%s>\n", local_node.name);

      // Read directory when it's not '.' or '..'
      if (strcmp (local_node.name, ".") != 0 && strcmp (local_node.name, "..") != 0) {
        readdir (&local_node, depth+1);
      }
    } else {
      if ( (local_node.flags & FS_BLOCKDEVICE) == FS_BLOCKDEVICE ||
           (local_node.flags & FS_CHARDEVICE) == FS_CHARDEVICE) {
        // This is a device
        kprintf ("%s  (Device %d:%d)\n", local_node.name, local_node.major_num, local_node.minor_num);
      } else {
        // This is a file
        kprintf ("%s  (%d bytes)\n", local_node.name, local_node.length);
      }
    }
  } // while readdir (root, length)

//  kprintf ("readdir() done\n");
}


/**
 *
 */
void kernel_setup (int stack_start, int total_sys_memory, const char *boot_params) {
  // Number of times the timer-interrupts is called.
  _kernel_ticks = 0;

  // Initialise memory and console first
  kmem_init (total_sys_memory);
  console_init ();

  // Show some welcome messages
  kprintf ("\n");
  kprintf ("Initializing CybOS kernel v%s.%s (%s)\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_COMPILER);
  kprintf ("This kernel was compiled at %s on %s\n", KERNEL_COMPILE_TIME, KERNEL_COMPILE_DATE);
  kprintf ("Available system memory: %dKB (%dMB)\n", total_sys_memory / 1024, total_sys_memory / (1024*1024));
  if (boot_params != NULL) kprintf ("Boot parameters: %s\n", boot_params);
  kprintf ("\n");


  /**
   * Init global kernel components that always need to be available
   */

  kprintf ("Init components: [ ");

  // Setup (new) global descriptor table
  kprintf ("GDT ");
  gdt_init ();

  // Initialize PIC interrupt controller
  kprintf ("PIC ");
  pic_init();

  // Initialize interrupt timer
  kprintf ("PIT ");
  pit_set_frequency (100);      // Set PIT frequency to 1000 ints per second (1ms apart)

  // Initialise timer
  kprintf ("TMR ");
  timer_init ();

  // Create interrupt and exception handlers
  kprintf ("IDT ");
  idt_init();

  // Setup paging
  kprintf ("PAG ");
  paging_init();

  /* Allocate a buffer for floppy DMA transfer. Needed here for now since we cannot
   * force <16MB allocation through the new VMM yet
   * @TODO: Remove this as soon as we are able to use kmalloc_dma () */
  kmalloc_pageboundary_physical (FDC_DMABUFFER_SIZE, (Uint32 *)&floppyDMABuffer);

  // Setup stack
  kprintf ("MEM ");
  heap_init();

  // Setup stack
  kprintf ("STK ");
  stack_init (stack_start);

  // Initialize DMA
  kprintf ("DMA ");
  dma_init ();

  // Initialise PCI
  kprintf ("PCI ");
  pci_init ();

  // Init device handler
  kprintf ("DEV ");
  device_init ();

  // Init virtual file system and global file systems present in the kernel
  kprintf ("VFS ");
  vfs_init ();
  cybfs_init ();
  devfs_init ();
  fat12_init ();
  // ext3_init ();  // @TODO: create ext3

  int ret = sys_mount (NULL, "devfs", "DEVICE", "/", 0);
  if (! ret) kpanic ("Error while mounting DevFS filesystem. Cannot continue!\n");

  // Start interrupts, needed because we now do IRQ's for floppy access
  sti ();

  // Init floppy disk controllers and drives
  kprintf ("FDC ");
  fdc_init ();      // Creates DEVICES:/FLOPPY* devices

  // Init harddisk controllers and drives (and partitions)
  kprintf ("IDE ");
  ide_init ();      // Creates DEVICES:/IDEC?D? devices and /IDEC?D?P? for partitions

  // Initialize multitasking environment
  kprintf ("TSK ");
  sched_init ();

  kprintf ("]\n");
  kprintf ("Kernel initialization done. Unable to free %d bytes.\n", _unfreeable_kmem);

  kprintf ("\n");

  // Start interrupts here
  sti ()
}


/**
 *
 */
void mount_root_system (const char *boot_params) {
  // Find root device or panic when not found
  char root_device_path[255];
  if (! get_boot_parameter (boot_params, "root=", (char *)&root_device_path)) {
    kpanic ("No root= parameter given. Cannot continue!\n");
  }

  // Find the root system type (no auto detect yet)
  char root_type[10] = "";    // Autodetect when empty
  get_boot_parameter (boot_params, "root_type=", (char *)&root_type);

  // Mount "ROOT" as our system
  int ret = sys_mount (root_device_path, root_type, "ROOT", "/", MOUNTOPTION_REMOUNT);
  if (! ret) kpanic ("Error while mounting root filesystem from '%s'. Cannot continue!\n", root_device_path);


  kprintf ("- MOUNT ROOT SYSTEM ---------------------\n");
  vfs_node_t *node = vfs_get_node_from_path ("ROOT:/");
  readdir (node, 0);
  kprintf ("-----------------------------------------\n");
}


char queue_find_item_str[20];
int queue_find_item (const void *item) {
  kprintf ("inside queue_find_item: Browsing item '%s'\n", item);
  if (strcmp (item, queue_find_item_str) == 0) {
    kprintf ("Found a match!\n");
    return 1;
  }
  return 0;
}

/**
 *
 */
void start_init (const char *boot_params) {
  // Default init program
  char init_prog[50] = "ROOT:/SYSTEM/INIT.BIN";

  // Get init from the command line (if given)
  get_boot_parameter (boot_params, "init=", (char *)&init_prog);

  tprintf ("!!!!Transfering control to user mode and starting: %s.\n", init_prog);

  if (! execve (init_prog, NULL, environ)) {
    tprintf ("**** Cannot execute init. Halting system!");
    for (;;);
  }

  // We cannot be here since execve will overwrite the current task
  kdeadlock();
}


/************************************
 * Returns 0-terminated boot parameter. Arg must be 'arg=' format.
 */
int get_boot_parameter (const char *boot_parameters, const char *arg, char *buffer) {
  // Find argument or return when not found
  char *c = strstr (boot_parameters, arg);
  if (c == NULL) return 0;

  // Start from end of 'key='
  c += strlen (arg);

  Uint32 l = 0;

  // If we started with a quote, end on quote, not a space
  char tc = ' ';
  if (*c == '\'' || *c == '"') {
    tc = *c;    // Skip begin quote
    c++;        // I can do C++ me...
  }

  // Find first space (or end of string)
  while (*(c+l) != tc && *(c+l) != 0) l++;

  // Copy and escape it into boot param return value
  strncpy(buffer, c, l);
  buffer[l] = 0;

  return 1;
}


/************************************
 * Switch to construct and deadlock the system
 */
void kdeadlock (void) {
  switch_console (_kconsole, 1);
  kprintf ("\nkdeadlock(): Deadlocked the system.\n");

  // Stop interrupts and halt processor. That should deadlock the system decently enough
  cli ();
  hlt ();

  // We should not be here. But in case we do, just "hang"
  for (;;) ;
}


/************************************
 * Only print when we can print
 */
int kprintf_help (char c, void **ptr) {
  // Bochs debug output
#ifdef __DEBUG__
  outb (0xE9, c);
#endif

  // Kernel messages go direct the construct
  con_putch (_kconsole, c); // Write to the construct

  // Also write to the current screen if it's not already on the main screen
  if (_current_task && _current_task->console->index != CON_KERNEL_IDX) con_putch (_current_task->console, c);
  return 0;
}


/************************************
 * Prints on the construct console (but we don't switch to it)
 */
void kprintf (const char *fmt, ...) {
  va_list args;

  va_start (args, fmt);
  (void)do_printf (fmt, args, kprintf_help, NULL);
  va_end (args);

  if (_kflush) con_flush (_kconsole);
}


/************************************
 * Disables kernel console flushing
 */
void knoflush () {
  _kflush = 0;
}


/************************************
 * Enabled kernel console flushing and forces a flush
 */
void kflush () {
  _kflush = 1;
  con_flush (_kconsole);
}


/************************************
 * Panics the system.
 */
void kpanic (const char *fmt, ...) {
  va_list args;

  cli();

  // Make sure we are currently seeing the construct
  switch_console (_kconsole, 1);

  kprintf ("\n[KRN] KERNEL PANIC: ");
  va_start (args, fmt);
  do_printf (fmt, args, kprintf_help, NULL);
  va_end (args);

  con_flush (_kconsole);
  kdeadlock();
}


/************************************
 * Prints on the construct console (but we don't switch to it)
 */
int tprintf_help (char c, void **ptr) {
  // Bochs debug output
#ifdef __DEBUG__
  outb (0xE9, c);
#endif

  // print char
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : : "a" (SYS_CONWRITE), "b" (c), "c" (0) );
  return 0;
}

void tprintf (const char *fmt, ...) {
  va_list args;

  va_start (args, fmt);
  (void)do_printf (fmt, args, tprintf_help, NULL);
  va_end (args);

  // Flush output
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : : "a" (SYS_CONFLUSH));
}


void do_tprintf (int dummy) {
  dummy += 1;
  tprintf ("2");
}


/****************************************************************************
 * Startup of the kernel.
 */
void kernel_entry (int stack_start, int total_sys_memory, const char *boot_params) {
  cli ();   // Disable interrupts

  kernel_setup (stack_start, total_sys_memory, boot_params);

  kprintf ("* Mounting root filesystem\n");
  mount_root_system (boot_params);

  kprintf ("* Switching to usermode\n");
  switch_to_usermode ();

  tprintf ("* Starting init program\n");

  // Child fork will run init (and does not return)
  if (! fork()) start_init (boot_params);

  // PID 0 idles when no running process could be found
  for (;;) idle ();
}

