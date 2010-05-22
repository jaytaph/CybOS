/******************************************************************************
 *
 *  File        : floppy.c
 *  Description : Floppy device
 *
 *
 *****************************************************************************/

#include "drivers/floppy.h"
#include "kernel.h"
#include "kmem.h"

/**
 * Functions that explicitly use output commands (like read,seek,reset etc), do not get
 * the fdc_drive_t argument but use the _currentDrive. This is because the controller
 * needs to be initialized to the correct drive.
 */

// Constants for drive driveinfo
const fdc_driveinfo_t driveinfo[8] = {
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 },  // No drive
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 },  // 360KB 5.25"
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 },  // 1.2MB 5.25"
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 },  // 720KB 3.5"
                                       { 0, 80, 2, 18, 0x1b, 2, 2, 8, 5, 0 },  // 1.44MB 3.5"
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 },  // 2.88MB 3.5"
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 },  // Unknown
                                       { 0,  0, 0,  0,    0, 0, 0, 0, 0, 0 }   // Unknown
                                     };

// Floppy drives types as taken from CMOS.  6 and 7 are not used.
const char *floppyTypes[] = { "no drive",
                              "360 KB 5.25 Drive",
                              "1.2 MB 5.25 Drive",
                              "720 KB 3.5 Drive",
                              "1.44 MB 3.5 Drive",
                              "2.88 MB 3.5 Drive",
                              "Unknown type",
                              "Unknown type"
                             };

// Flag to be set as soon as IRQ happens (set by IRQ handler)
static volatile Uint8 fdc_receivedIRQ = 0;

// Buffer (in DMA memory <16MB) for DMA transfers, TODO: @allocate per controller
char *floppyDMABuffer;

// There can be only 1 drive active since all drives use IRQ6/DMA2. This var holds
// the currently active drive.
fdc_drive_t *_currentDrive;


/**
 * Waits and returns 1 when FDC is ready or 0 when not ready on time
 */
int fdc_wait_status_ready (void) {
  int i, status;

  // Wait until FDC is ready. Retry multiple times since it might take a while
  for (i=0; i!=500; i++) {
    status = inb (_currentDrive->fdc->baseAddress + FR_MAIN_STATUS_REGISTER);
    if (status & MSR_MASK_DATAREG) return 1;
  }
  return 0;
}


/**
 * Wait until a floppy IRQ happens
 */
void fdc_wait_for_irq (void) {
  // Wait until it's not 0
  while (! fdc_receivedIRQ) {
    // @TODO: Instead of polling, we should sleep()/reschedule. Wake up on IRQ6

    // Make sure IRQ's are enabled when waiting for IRQ
    if (! ints_enabled ()) kpanic ("Waiting for IRQ, but interrupts are not enabled");
  };

  // Reset to 0
  fdc_receivedIRQ = 0;
}


/**
 * Send commands to FDC
 */
void fdc_send_command (Uint8 command) {
  if (! fdc_wait_status_ready()) kpanic ("Floppy send error\n");

//  kprintf ("out(%03X : %02X)\n", _currentDrive->fdc->baseAddress + FR_DATA_FIFO, command);
  return outb (_currentDrive->fdc->baseAddress + FR_DATA_FIFO, command);
}


/**
 * Receives data from FDC or deadlocks when device is not ready (@TODO: Should not deadlock)
 */
Uint32 fdc_recv_data (void) {
  if (! fdc_wait_status_ready()) kpanic ("Floppy recv error\n");

  Uint8 data = inb (_currentDrive->fdc->baseAddress + FR_DATA_FIFO);
//  kprintf ("in(%03X : %02X)\n", _currentDrive->fdc->baseAddress + FR_DATA_FIFO, data);
  return data;
}


/**
 * Floppy interrupt handler. Does nothing except setting flag
 */
void floppy_interrupt (regs_t *r) {
  fdc_receivedIRQ = 1;
}


/**
 * Sets the new data when switching drive (if needed or forced)
 */
int fdc_switch_active_drive (fdc_drive_t *drive, int force_select) {
  // If this drive is currently selected, do not do anything...
  if (! force_select &&
      _currentDrive->fdc->controllerNum == drive->fdc->controllerNum &&
      _currentDrive->driveNum == drive->driveNum) {
//    kprintf ("fdc_switch_active_drive (%d/%d): already there...\n", drive->fdc->controllerNum, drive->driveNum);
    return 0;
  }

//  kprintf ("fdc_switch_active_drive (%d)", drive->driveNum);

  // Set the current drive
  _currentDrive = drive;

//  kprintf ("Specify\n");

    // Initialise drive mode (needed on every switch)
  Uint32 data = 0;
  fdc_send_command (FC_SPECIFY);

  data = ( (drive->driveinfo.stepRate & 0xf) << 4) | (drive->driveinfo.unloadTime & 0xf);
  fdc_send_command (data);

  data = (drive->driveinfo.loadTime) << 1 | drive->driveinfo.usePIO;
  fdc_send_command (data);

  // Set DMA info
  dma_set_address (DMA_FLOPPY_CHANNEL, drive->fdc->dma.buffer);
  dma_set_size (DMA_FLOPPY_CHANNEL, drive->fdc->dma.size);

  return 1;
}



/**
 * Enabled or disables specified drive motor
 */
void fdc_control_motor (Uint8 motorOn) {
  if (motorOn) {
    // All drive bits are sequential so we can shift right from drive 0
    Uint8 motor = (DOR_MASK_DRIVE0_MOTOR << _currentDrive->driveNum);   // Find which motor to control
    outb (_currentDrive->fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, _currentDrive->driveNum | motor | DOR_MASK_RESET | DOR_MASK_DMA);

    // sleep a bit to spin up the motor
    int i; for (i=0; i!=200000; i++) ;
  } else {
    // Motor Off
    outb (_currentDrive->fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, _currentDrive->driveNum | DOR_MASK_RESET | DOR_MASK_DMA);
  }
}

/**
 * Check interrupt status of FDC
 */
void fdc_check_interrupt_status (Uint32 *statusRegister, Uint32 *currentCylinder) {
  fdc_send_command (FC_CHECK_INT);
  *statusRegister = fdc_recv_data ();
  *currentCylinder = fdc_recv_data ();
}

/**
 * Calibrate floppy disk by setting the cylinder to cylinder 0
 */
int fdc_calibrate_drive () {
  Uint32 st0, cyl;
  int i;

  // Set motor
  fdc_control_motor (1);

//  kprintf ("Calibrate drive %d/%d\n", _currentDrive->fdc->controllerNum, _currentDrive->driveNum);

  // Do a few times
  for (i=0; i<10; i++) {
    fdc_send_command (FC_CALIBRATE);
    fdc_send_command (_currentDrive->driveNum);
    fdc_wait_for_irq ();
    fdc_check_interrupt_status (&st0, &cyl);

    // The drive is on cylinder 0, so return
    if (cyl == 0) {
      // Shut off motor
      fdc_control_motor (0);
      return 1;
    }
  }

  // Tried a few times, but floppy drive is still not on cylinder 0, return error
  fdc_control_motor (0);
  return 0;
}





/**
 * Resets the floppy controller
 */
void fdc_reset_controller (fdc_t *fdc) {
  Uint32 st, cyl;
  int i;

  // Disable controller
  outb (fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, 0x00);

  // Enable controller
  outb (fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, DOR_MASK_DRIVE0 | DOR_MASK_RESET | DOR_MASK_DMA);

  // Wait for IRQ to happen
  fdc_wait_for_irq ();

  // Sense interrupt status (4 times, why? mechanical delay reason?)
  for (i=0; i!=4; i++) fdc_check_interrupt_status (&st, &cyl);

  // Set controller to 500Kpbs
  outb (fdc->baseAddress + FR_CONFIGURATION_CONTROL_REGISTER, 0x00); // Set to 500 Kbps

  // Reset individual drives
  for (i=0; i!=2; i++) {
    fdc_switch_active_drive (&fdc->drives[i], 0);
    fdc_calibrate_drive ();
  }
}


/**
 * Return floppy controller version
 */
Uint8 fdc_get_controller_version (void) {
  fdc_send_command (FC_VERSION);

  Uint8 controllerVersion = fdc_recv_data ();
  switch (controllerVersion) {
    case 0xFF: // No controller. Does not matter
               break;
    case 0x80: kpanic ("NEC controller is not supported\n");
               break;
    case 0x81: kpanic ("VMware controller is not supported\n");
               break;
    case 0x90:  // This controller IS supported!
               break;
    default:   kpanic("Unknown controller [%d]: not supported\n", controllerVersion);
               break;
  }

  return controllerVersion;
}

/**
 * Converts LBA sector to CHS format
 */
void fdc_convert_LBA_to_CHS (fdc_drive_t *drive, Uint32 lba_sector, Uint32 *cylinder, Uint32 *head, Uint32 *sector) {
  *cylinder = lba_sector / (drive->driveinfo.sectorsPerTrack * drive->driveinfo.maxHead);
  Uint32 tmp = lba_sector % (drive->driveinfo.sectorsPerTrack * drive->driveinfo.maxHead);
  *head = tmp / drive->driveinfo.sectorsPerTrack;
  *sector = tmp % drive->driveinfo.sectorsPerTrack + 1;
}

/**
 * Reads CHS data
 */
void fdc_read_floppy_sector_CHS (Uint32 cylinder, Uint32 head, Uint32 sector) {
  Uint32 st0, cyl;

//  kprintf ("READ_FLOPPY_SECTOR_CHS\n");

  // Tell DMA we are reading from buffer
  dma_set_read (DMA_FLOPPY_CHANNEL);

  // Read in a sector
  fdc_send_command (FC_READ_SECT | FDC_MULTITRACK | FDC_DENSITY);
  fdc_send_command (head << 2 | _currentDrive->driveNum);
  fdc_send_command (cylinder);
  fdc_send_command (head);    // Strange, we already send this info, but FDC needs it twice..
  fdc_send_command (sector);
  fdc_send_command (_currentDrive->driveinfo.sectorDTL);
  fdc_send_command (( ( sector + 1 ) >= _currentDrive->driveinfo.sectorsPerTrack ) ? _currentDrive->driveinfo.sectorsPerTrack : sector + 1 );
  fdc_send_command (_currentDrive->driveinfo.gap3 );
  fdc_send_command (0xff);

  // Wait for IRQ to happen (ie: sector read by DMA is completed)
  fdc_wait_for_irq ();

  // Read status information and save into result
  _currentDrive->result.st0    = fdc_recv_data ();
  _currentDrive->result.st1    = fdc_recv_data ();
  _currentDrive->result.st2    = fdc_recv_data ();
  _currentDrive->result.st3    = fdc_recv_data ();
  _currentDrive->result.track  = fdc_recv_data ();
  _currentDrive->result.head   = fdc_recv_data ();
  _currentDrive->result.sector = fdc_recv_data ();
  _currentDrive->result.size   = fdc_recv_data ();

  fdc_check_interrupt_status (&st0, &cyl);
}


/**
 * Seek floppy cylinder
 */
int fdc_seek_floppy_cylinder (Uint32 cylinder, Uint8 head) {
  Uint32 st0, cyl;
  int i;

  for (i=0; i!=10; i++) {
//    kprintf ("seek_floppy_cylinder (%d, %d)\n", _currentDrive->driveNum, cylinder, head);

    fdc_send_command (FC_SEEK);
    fdc_send_command ((head) << 2 | _currentDrive->driveNum);
    fdc_send_command (cylinder);

    fdc_wait_for_irq ();
    fdc_check_interrupt_status (&st0, &cyl);
//    kprintf ("ST0: %02X CYL: %02X\n", st0, cyl);
    if (cyl == cylinder) return 1;      // We are at the correct cylinder
  }
  // Not (yet) there..
  return 0;
}


/**
 * Reads SINGLE sector from drive (not very optimized when reading large data blocks)
 */
void fdc_read_floppy_sector (fdc_drive_t *drive, Uint32 lba_sector, char *buffer) {
  Uint32 c,h,s;

  // Switch active drive
  fdc_switch_active_drive (drive, 0);

  // Start motor
  fdc_control_motor (1);

//  kprintf ("Read floppy sector: %08X\n", lba_sector);

  // Convert LBA sector to CHS
  fdc_convert_LBA_to_CHS (_currentDrive, lba_sector, &c, &h, &s);

//  kprintf ("C: %d  H: %d   S: %d\n", c, h, s);

  // Seek correct cylinder (and head)
  while (! fdc_seek_floppy_cylinder (c, h)) ;

  // Read CHS sector
  fdc_read_floppy_sector_CHS (c, h, s);

  // Copy the data from DMA buffer into the actual buffer
  memcpy (buffer, fdc->dma.buffer, fdc->dma.size);

  // Shut down motor
  fdc_control_motor (0);
}



Uint32 fdc_block_read (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("fdc_block_read(%d, %d, %d, %d, %08X)\n", major, minor, offset, size, buffer);
  return 0;
}
Uint32 fdc_block_write (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("fdc_block_write(%d, %d, %d, %d, %08X)\n", major, minor, offset, size, buffer);
  kprintf ("write to floppy not supported yet\n");
  return 0;
}
void fdc_block_open(Uint8 major, Uint8 minor) {
  kprintf ("fdc_block_open(%d, %d)\n", major, minor);
  // Doesn't do anything. Device already open?
}
void fdc_block_close(Uint8 major, Uint8 minor) {
  kprintf ("fdc_block_close(%d, %d)\n", major, minor);
  // Doesn't do anything. Device never closes?
}
void fdc_block_seek(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction) {
  kprintf ("fdc_block_seek(%d, %d, %d, %d)\n", major, minor, offset, direction);
  // Doesn't do anything.
}


/**
 *
 */
void fdc_init_drive (fdc_t *fdc, Uint8 driveNum, Uint8 driveType) {
//  kprintf ("fdc_switch_active_drive (%d/%d)\n", fdc->controllerNum, driveNum);
  // Can only do drive 0 or drive 1
  if (driveNum > 1) return;

  // Drive type must be 0..7
  if (driveType > 8) return;

  // Copy driveinfo data for this drive
  memcpy ((char *)&fdc->drives[driveNum].driveinfo, (char *)&driveinfo[driveType], sizeof (fdc_drive_t));

  fdc->drives[driveNum].driveType = driveType;    // Drive number on the FDC (0 or 1)
  fdc->drives[driveNum].driveNum = driveNum;
  fdc->drives[driveNum].fdc = fdc;                // Backwards link, needed to find FDC from a fdc_drive_t structure

  // Set default CHS values
  fdc->drives[driveNum].currentCylinder = 0;
  fdc->drives[driveNum].currentHead = 0;
  fdc->drives[driveNum].currentSector = 0;


  // Register device so we can access it
  device_t *device = (device_t *)kmalloc (sizeof (device_t));
  device->majorNum = DEV_MAJOR_FDC;
  device->minorNum = (fdc->controllerNum * 2) + driveNum;

  device->read = fdc_block_read;
  device->write = fdc_block_write;
  device->open = fdc_block_open;
  device->close = fdc_block_close;
  device->seek = fdc_block_seek;

  // Create device name
  // @TODO: use sprintf(filename, "FLOPPY%d", device->minorNum);
  char filename[11];
  strcpy ((char *)&filename, "FLOPPY");
  filename[6] = '0' + device->minorNum;

  device_register (device, filename);

/*
  kprintf ("driveinfo\n");
  kprintf ("DI->maxCylinder %d\n", fdc->drives[driveNum].driveinfo.maxCylinder);
  kprintf ("DI->maxHead     %d\n", fdc->drives[driveNum].driveinfo.maxHead);
  kprintf ("DI->spt         %d\n", fdc->drives[driveNum].driveinfo.sectorsPerTrack);
  kprintf ("DI->gap3        %d\n", fdc->drives[driveNum].driveinfo.gap3);
  kprintf ("DI->sectorDTL   %d\n", fdc->drives[driveNum].driveinfo.sectorDTL);
  kprintf ("DI->stepRate    %d\n", fdc->drives[driveNum].driveinfo.stepRate);
  kprintf ("DI->loadTime    %d\n", fdc->drives[driveNum].driveinfo.loadTime);
  kprintf ("DI->unloadTime  %d\n", fdc->drives[driveNum].driveinfo.unloadTime);
  kprintf ("DI->usePIO      %d\n", fdc->drives[driveNum].driveinfo.usePIO);
*/

  // Calibrate drive
  fdc_switch_active_drive (&fdc->drives[driveNum], 1);
  fdc_calibrate_drive ();
}


/**
 *
 */
void fdc_init_controller (fdc_t *fdc, Uint8 controllerNum, Uint32 baseAddress, char *floppyDMABuffer, Uint16 floppyDMABufferSize) {
  Uint8 driveType, i;

  // Set DMA info
  // @TODO: How do we decide how much DMA memory we need? or should we just allocate 64KB always?
  fdc->dma.buffer = (char *)floppyDMABuffer;
  fdc->dma.size = floppyDMABufferSize;

  // Set controller number (first or second controller)
  fdc->controllerNum = controllerNum;

  // Set base address
  fdc->baseAddress = baseAddress;

  // Set default drives to "no drive available"
  fdc->drives[0].driveType = 0x00;
  fdc->drives[1].driveType = 0x00;

  // Make sure both drives points to the correct FDC. Otherwise
  fdc->drives[0].fdc = fdc;
  fdc->drives[1].fdc = fdc;

  /* Set temporary drive so fdc_get_controller_version works. This is a bit of a hack but
   * done because otherwise we would need 4/5 separate functions that do not use _currentDrive
   * but a FDC taken from the parameter. It's more cleaner to do it this way */
  _currentDrive = &fdc->drives[0];

  // Get controller version
  fdc->version = fdc_get_controller_version ();

  // Don't init drives when no controller is present
  if (fdc->version == 0xFF) return;

  // Read CMOS for number of floppy disks (@TODO: how to read status for second floppy controller?)
  outb (0x70, 0x10);
  for (i=0; i!=100; i++) ;     // Delay a bit
  driveType = inb (0x71);

//  kprintf ("\n");
//  kprintf ("Controller %d: master drive: %s\n", controllerNum, floppyTypes[((driveType >> 4) & 0x7)]);
//  kprintf ("Controller %d: slave drive: %s\n", controllerNum, floppyTypes[(driveType & 0x7)]);

  // Init drive 0 and 1, or 2 and 3 when we are the second controller
  fdc_init_drive (fdc, (controllerNum * 2) + 0, (driveType >> 4) & 0x7);
  fdc_init_drive (fdc, (controllerNum * 2) + 1, driveType & 0x7);

  // Do a complete reset controller (and the drives)
  fdc_reset_controller (fdc);
}

/**
 *
 */
void fdc_init (void) {
  int i, fdcBaseAddr;

  // Try to initialise 2 controllers
  for (i=0; i!=2; i++) {
    fdcBaseAddr = (i == 0) ? FDC0_BASEADDR : FDC1_BASEADDR;
    fdc_init_controller (&fdc[i], i, fdcBaseAddr, floppyDMABuffer, FDC_DMABUFFER_SIZE);
  }
}