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
#include "drivers/floppy.h"
#include "vfs.h"
#include "vfs/fat12.h"
#include "vfs/cybfs.h"

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
  vfs_dirent_t *unsafe_dirent;
  vfs_node_t *unsafe_node;
  vfs_dirent_t local_dirent;
  vfs_node_t local_node;
  int index = 0;
  int j;

  while (unsafe_dirent = vfs_readdir (root, index), unsafe_dirent != NULL) {
    index++;

    // Copy to local scope
    memcpy (&local_dirent, unsafe_dirent, sizeof (vfs_dirent_t));

    // File cannot be found (huh?)
    unsafe_node = vfs_finddir (root, local_dirent.name);
    if (! unsafe_node) continue;

    // Copy to local scope
    memcpy (&local_node, unsafe_node, sizeof (vfs_node_t));

    for (j=0; j!=depth; j++) kprintf ("  ");
    if ((local_node.flags & FS_DIRECTORY) == FS_DIRECTORY)  {
      kprintf ("<%s>\n", local_node.name);
      if (local_node.name[0] != '.') {
        readdir (&local_node, depth+1);
      }
    } else {
      kprintf ("%s\n", local_node.name);

      if (local_node.length != 0) {
        char buf[512];
        Uint32 size = vfs_read (&local_node, 0, 512, (char *)&buf);
        kprintf ("--- File contents (%d bytes) -----------------\n", size);
        kprintf ("%s", buf);
        kprintf ("--- End File contents -------------\n");
      }
    }
  }
}

/****************************************************************************
 * Startup of the kernel.
 */
void kernel_entry (int stack_start, int total_sys_memory, char *boot_params) {
  // No interruptions until we decide to start again
  cli ();

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
  kprintf ("TIM ");
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

  kprintf ("MEM ");
  heap_init();

  // @TODO: Must this be here, or later on when we are right before switching?
  _current_pagedirectory = clone_pagedirectory (_kernel_pagedirectory);
  set_pagedirectory (_current_pagedirectory);

  kprintf ("STK ");
  stack_init (stack_start);

  kprintf ("DMA ");
  dma_init ();

  kprintf ("VFS ");
  vfs_init ();
  cybfs_init ();      // Creates CybFS root system

  kprintf ("DEV ");
  device_init ();     // Creates /DEVICES

  // Start interrupts, needed because we now do IRQ's for floppy access
  sti ();

  // Init floppy disk controllers and drives
  kprintf ("FDC ");
  fdc_init ();

  kprintf ("FAT ");
  fat12_init ();

  // Initialize multitasking environment
  kprintf ("TSK ");
  sched_init ();

  kprintf ("]\n");
  kprintf ("Kernel initialization done. Unable to free %d bytes.\n", _unfreeable_kmem);


  kprintf ("\n");
  kprintf ("File system drivers loaded: \n");
  int i;
  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    if (! vfs_systems[i].tag[0]) continue;
    kprintf ("'%10s' => %s\n", vfs_systems[i].tag, vfs_systems[i].name);
  }
  kprintf ("\n");


  for (;;) ;


  // Display root directory hierarchy
  readdir (vfs_root, 0);

  for (;;) ;


  /*
   * All done with kernel initialization. Mount the root filesystem to the correct one. This is
   * passed by the boot loader.
   */
  kprintf ("Mounting root filesystem\n");

  // Find root device or panic when not found
  char root_device[255];
  if (! get_boot_parameter (boot_params, "root=", (char *)&root_device)) {
    kpanic ("No root= parameter given. Cannot continue!\n");
  }

  // Find the root system type and mount the system to root
  char root_type[10] = "";    // Autodetect when empty
  get_boot_parameter (boot_params, "root_type=", (char *)&root_type);

  int ret = sys_mount (root_device, "/PROGRAMS", root_type);
  if (! ret) kpanic ("Error while mounting root filesystem. Cannot continue!\n");

  // Display root directory hierarchy
  readdir (vfs_root, 0);


  /*
   * We are mounted, next, jump to usermode, fork into a new task (INIT) which
   * executes /SYSTEM/INIT.
   */

  char init_prog[50] = "/SYSTEM/INIT";
  get_boot_parameter (boot_params, "init=", (char *)&init_prog);
  kprintf ("Transfering control to user mode and starting %s.\n\n\n", init_prog);

  // Switch to ring3 and start interrupts automatically
  switch_to_usermode ();

  int pid = fork ();

  if (pid == 0) {
    // @TODO: sys_exec (init_prog);
    strncpy (_current_task->name, "init", 30);

    pid = fork ();
    if (pid == 0) {
        strncpy (_current_task->name, "app01", 30);
        for (;;) {
            sleep (2500);

            tprintf ("\n\n");
            tprintf ("PID  PPID TASK                STAT  PRIO  KTIME     UTIME\n");
            task_t *t;
            for (t=_task_list; t!=NULL; t=t->next) {
                tprintf ("%04d %04d %-17s      %c  %4d  %08X  %08X\n", t->pid, t->ppid, t->name, t->state, t->priority, LO32(t->ktime), LO32(t->utime));
            }
            tprintf ("\n");
        }
    } else {
        tprintf ("Doing child (%d)\n", getpid());
        for (;;) {
            int i;
            for (i=0; i!=500; i++) tprintf ("Q");
            tprintf ("ZZZZ");
            sleep (1000);
            tprintf ("Wakeup");
        }
    }
  }

  tprintf ("Doing parent (%d)\n", getpid());

  // This is the idle task (PID 0)
  for (;;) idle ();
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
