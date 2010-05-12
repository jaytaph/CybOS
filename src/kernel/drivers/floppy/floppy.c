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
 #include "dma.h"

 const char *floppyDescriptions[] = { "no drive",
                                      "360 KB 5.25 Drive",
                                      "1.2 MB 5.25 Drive",
                                      "720 KB 3.5 Drive",
                                      "1.44 MB 3.5 Drive",
                                      "2.88 MB 3.5 Drive",
                                      "Unknown type",
                                      "Unknown type" };

// Flag to be set as soon as IRQ happens (set by IRQ handler)
static volatile Uint8 fdc_receivedIRQ = 0;

// Buffer (in DMA memory <16MB) for DMA transfers
char *dma_floppy_buffer;

// There can be only 1 drive active since all drives use IRQ6/DMA2. This var holds
// the current active drive.
fdc_drive_t _currentDrive;


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
void fdc_wait_floppy_irq (void) {
    // Wait until it's not 0
    while (! fdc_receivedIRQ) ;

    // Reset to 0
    fdc_receivedIRQ = 0;
}


/**
 * Send commands to FDC
 */
void fdc_send_command (Uint8 command) {
    if (! fdc_wait_status_ready()) {
        kprintf ("Floppy send error\n");
        kdeadlock ();
    }

    kprintf ("SC(%02X)\n", command);
    return outb (_currentDrive->fdc->baseAddress + FR_DATA_FIFO, command);
}


/**
 * Receives data from FDC or deadlocks when device is not ready (@TODO: Should not deadlock)
 */
Uint32 fdc_recv_data (void) {
    if (! fdc_wait_status_ready()) {
        kprintf ("Floppy recv error\n");
        kdeadlock ();
    }

    Uint8 data = inb (_currentDrive->fdc->baseAddress + FR_DATA_FIFO);
    kprintf ("RD(%02X)\n", data);
    return data;
}


/**
 * Floppy interrupt handler. Does nothing except setting flag
 */
void floppy_interrupt (regs_t *r) {
    fdc_receivedIRQ = 1;
}

/**
 * Sets the new data when switching drive (if needed)
 */
void fdc_select_drive (fdc_drive_t *drive) {
    // If this drive is currently selected, do not do anything...
    if (_currentDrive->fdc->currentController == drive->fdc->currentController &&
        _currentDrive->driveNum == drive->driveNum) return;

    _currentDrive = drive;

	Uint32 data = 0;
	fdc_send_command (FC_SPECIFY);

	data = ( (drive->geometry.step_rate & 0xf) << 4) | (drive->geometry.unload_time & 0xf);
	fdc_send_command (data);

	data = (drive->geometry.load_time) << 1 | drive->geometry.use_pio;
    fdc_send_command (data);
}



/**
 * Enabled or disables specified drive motor
 */
void fdc_control_motor (fdc_drive_t *drive, Uint8 status) {
    if (status == MOTOR_ON) {
        // Doable because all drive bits are sequential
        Uint8 motor = (DOR_MASK_DRIVE0_MOTOR << drive->driveNum);   // Find which motor to control
        outb (drive->fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, drive->driveNum | motor | DOR_MASK_RESET | DOR_MASK_DMA);

        // sleep a bit to spin up the motor
        int i; for (i=0; i!=200000; i++) ;
    } else {
        outb (drive->fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, drive->driveNum | DOR_MASK_RESET | DOR_MASK_DMA);
    }
}

/**
 * Check interrupt status of FDC
 */
void fdc_check_interrupt_status (fdc_t *fdc, Uint32 *statusRegister, Uint32 *currentCylinder) {
    fdc_send_command (fdc, FC_CHECK_INT);
    *statusRegister = fdc_recv_data (fdc);
    *currentCylinder = fdc_recv_data (fdc);
}

/**
 * Calibrate floppy disk by setting the cylinder to cylinder 0
 */
int fdc_calibrate_drive (fdc_drive_t *drive) {
    Uint32 st0, cyl;
    int i;
    fdc_t *fdc = drive->fdc;

    // Set motor
    control_motor (drive, MOTOR_ON);

    // Do a few times
    for (i=0; i<10; i++) {
        fdc_send_command (fdc, FC_CALIBRATE);
        fdc_send_command (fdc, drive->driveNum);
        wait_floppy_irq ();
        check_interrupt_status (fdc, &st0, &cyl);

        // The drive is on cylinder 0, so return
        if (cyl == 0) {
            // Shut off motor
            control_motor (drive, MOTOR_OFF);
            return 1;
        }
    }

    // Tried a few times, but floppy drive is still not on cylinder 0, return error
    control_motor (drive, MOTOR_OFF);
    return 0;
}


/**
 *
 */
void fdc_init_drive (fdc_t *fdc, Uint8 driveNum, Uint8 driveType) {
    // Can only do drive 0 or drive 1 (@TODO: what about drive 2 and 3?)
    if (driveNum > 1) return;

    // Drive type must be 0..7
    if (driveType > 8) return;

    // Copy geometry data
    memcpy ((char *)&fdc->drives[driveNum].geometry, (char *)&disk_geometries[driveType], sizeof (fdc_drive_t));
    fdc->drives[driveNum].driveType = driveType;    // Drive number on the FDC (0 or 1)
    fdc->drives[driveNum].fdc = fdc;    // Backwards link, needed to find FDC from a fdc_drive_t structure

    // Set default CHS values
    fdc->drives[driveNum].currentCylinder = 0;
    fdc->drives[driveNum].currentHead = 0;
    fdc->drives[driveNum].currentSector = 0;

    // Calibrate drive
    calibrate_drive (&fdc->drives[driveNum]);
}



/**
 * This will set the drive data for the CURRENT drive. Since every drive shares this info
 * we need to set it everytime we change drive.
 *
 * Example:
 *   Drive 0: 1.44MB
 *   Drive 1: 2.88MB
 *
 * When reading from drive 0, we need to specify this, when reading from drive 1 we need
 * to call with drive 1 data.
 */
/*
void obs_set_drive_data (Uint32 step_rate, Uint32 load_time, Uint32 unload_time, Uint8 use_pio) {
	Uint32 data = 0;
	fdc_send_command (FC_SPECIFY);

	data = ( (step_rate & 0xf) << 4) | (unload_time & 0xf);
	fdc_send_command (data);

	data = (load_time) << 1 | use_pio;
	kprintf ("Data: %02X\n", data);
    fdc_send_command (data);
}
*/


/**
 *
 */
void reset_drive (fdc_drive_t *drive) {
  // @TODO:   MAKE THIS WORK
//    // We should set this data depending on the drive type we detected  (1.44MB etc)
//    set_drive_data (3, 16, 240, 0);
//    calibrate_floppy_drive (0);
//
//    kprintf ("FDC reset done\n");
}



/**
 * Resets the floppy controller
 */
void reset_controller (fdc_t *fdc) {
    Uint32 st, cyl;
    int i;

    kprintf ("Resetting floppy controler: \n");

    // Disable controller
    outb (fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, 0x00);

    // Enable controller
    outb (fdc->baseAddress + FR_DIGITAL_OUTPUT_REGISTER, DOR_MASK_DRIVE0 | DOR_MASK_RESET | DOR_MASK_DMA);

    // Wait for IRQ to happen
    wait_floppy_irq ();

    // Sense interrupt status (4 times, why? mechanical delay reason?)
    for (i=0; i!=4; i++) check_interrupt_status (fdc, &st, &cyl);

    // Set controller to 500Kpbs
    outb (fdc->baseAddress + FR_CONFIGURATION_CONTROL_REGISTER, 0x00); // Set to 500 Kbps

    // Reset individual drives
    for (i=0; i!=2; i++) reset_drive (&fdc->drives[i]);
}





/**
 * Return floppy controller version
 */
Uint8 get_controller_version (fdc_t *fdc) {
    kprintf ("Floppy controller %d: ", fdc->controllerNum);
    send_command (fdc, FC_VERSION);

    Uint8 controllerVersion = fdc_recv_data (fdc);
    switch (controllerVersion) {
      case 0xFF: kprintf(" no controller found\n");
                 break;
      case 0x80: kprintf(" NEC controller\n");
                 break;
      case 0x81: kprintf(" VMware controller\n");
                 break;
      case 0x90: kprintf(" enhanced controller\n");
                 break;
      default:   kprintf(" unknown controller [%d]\n", controllerVersion);
                 break;
    }

    return controllerVersion;
}

/**
 * Converts LBA sector to CHS format
 */
void convert_LBA_to_CHS (fdc_drive_t *drive, Uint32 lba_sector, Uint32 *cylinder, Uint32 *head, Uint32 *sector) {
   *cylinder = lba_sector / (drive->geometry.sectorsPerTrack * 2 );
   *head = ( lba_sector % ( drive->geometry.sectorsPerTrack * 2 ) ) / ( drive->geometry.sectorsPerTrack );
   *sector = lba_sector % drive->geometry.sectorsPerTrack + 1;
}

/**
 * Reads CHS data
 */
void read_floppy_sector_CHS (fdc_drive_t *drive, Uint32 cylinder, Uint32 head, Uint32 sector) {
	Uint32 st0, cyl;

	kprintf ("READ_FLOPPY_SECTOR_CHS\n");

	//! read in a sector
	send_command (drive->fdc, FC_READ_SECT | FDC_MULTITRACK | FDC_SKIP | FDC_DENSITY);
	send_command (drive->fdc, head << 2 | drive->driveNum);
	send_command (drive->fdc, cylinder);
	send_command (drive->fdc, head);
	send_command (drive->fdc, sector);
	send_command (drive->fdc, drive->geometry.sectorDTL );
	send_command (drive->fdc,  ( ( sector + 1 ) >= drive->geometry.sectorsPerTrack ) ? drive->geometry.sectorsPerTrack : sector + 1 );
	send_command (drive->fdc, drive->geometry.gap3 );
	send_command (drive->fdc, 0xff);

	// Wait for IRQ to happen (ie: sector read by DMA)
	wait_floppy_irq ();

	// Read status information and save into result
	drive->result.st0    = fdc_recv_data (drive->fdc);
	drive->result.st1    = fdc_recv_data (drive->fdc);
	drive->result.st2    = fdc_recv_data (drive->fdc);
	drive->result.st3    = fdc_recv_data (drive->fdc);
	drive->result.track  = fdc_recv_data (drive->fdc);
	drive->result.head   = fdc_recv_data (drive->fdc);
	drive->result.sector = fdc_recv_data (drive->fdc);
	drive->result.size   = fdc_recv_data (drive->fdc);

	fdc_check_interrupt_status (drive->fdc, &st0, &cyl);
}


/**
 * Seek floppy track
 */
int seek_floppy_track (fdc_drive_t *drive, Uint32 cylinder, Uint32 head) {
    Uint32 st0, cyl;
    int i;

    kprintf ("seek_floppy_track (%d, %d, %d)", drive->driveNum, cylinder, head);
    for (i=0; i!=10; i++) {
        send_command (drive->fdc, FC_SEEK);
        send_command (drive->fdc, (head) << 2 | drive->driveNum);
        send_command (drive->fdc, cylinder);

        wait_floppy_irq ();
        check_interrupt_status (drive->fdc, &st0, &cyl);
        kprintf ("ST0: %02X CYL: %02X\n", st0, cyl);
        if (cyl == cylinder) return 1;      // We are at the correct cylinder
    }

    // Not (yet) there..
    return 0;
}


/**
 * Reads sector from drive
 */
void read_floppy_sectors (fdc_drive_t *drive, Uint32 lba_sector, Uint8 sector_count) {
    Uint32 c,h,s;
    int i;

    // Start motor
    control_motor (drive, MOTOR_ON);

    for (i=0; i!=sector_count; i++, lba_sector++) {
        kprintf ("Read floppy sector %d:%08X\n", drive, lba_sector);

        // Convert LBA sector to CHS
        convert_LBA_to_CHS (drive, lba_sector, &c, &h, &s);

        kprintf ("C: %d  H: %d   S: %d\n", c, h, s);

        // Seek correct cylinder (and head)
        while (! seek_floppy_track (drive, c, h)) ;

        // Read CHS sector
        read_floppy_sector_CHS (drive, h, c, s);
    }

    // Shut down motor
    control_motor (drive, MOTOR_OFF);

    return;
}


/**
 *
 */
void fdc_init_controller (fdc_t *fdc, Uint8 controllerNum, Uint16 baseAddress, char *dma_floppy_buffer) {
    Uint8 driveType, i;

    // Set DMA info
    fdc->dma.buffer = (char *)dma_floppy_buffer;
    fdc->dma.page = HI16(LO8((Uint32)dma_floppy_buffer)) & 0x0F;
    fdc->dma.high = LO16(HI8((Uint32)dma_floppy_buffer));
    fdc->dma.high = LO16(LO8((Uint32)dma_floppy_buffer));

    // Set controller number (first or second controller)
    fdc->controllerNum = controllerNum;

    // Get controller version
    fdc->version = get_controller_version (fdc);

    fdc->drives[0].driveType = 0x00;        // Set default to no drive
    fdc->drives[1].driveType = 0x00;        // Set default to no drive

    // Don't init drives when no controller is present
    if (fdc->version == 0xFF) return;

    // Read CMOS for number of floppy disks (@TODO: how to read status for second floppy controller?)
    outb (0x70, 0x10);
    for (i=0; i!=100; i++) ;     // Delay a bit
    driveType = inb (0x71);

    kprintf ("\n");
    kprintf ("Floppy disk types: %02X\n", driveType);
    kprintf ("Detected master floppy: %s\n", floppyDescriptions[((driveType >> 4) & 0x7)]);
    kprintf ("Detected slave floppy: %s\n", floppyDescriptions[(driveType & 0x7)]);

    // Init drive 0 and 1, or 2 and 3 when we are the second controller
    fdc_init_drive (fdc, startDriveNum + 0, (driveType >> 4) & 0x7);
    fdc_init_drive (fdc, startDriveNum + 1, driveType & 0x7);

    // Do a complete reset controller (and the drives)
    reset_controller (fdc);
}


/**
 *
 */
void fdc_init (void) {
    int i, baseaddr;

    // Needed since floppy init depends on IRQ's
    sti ();

    // Try to initialise 2 controllers
    for (i=0; i!=2; i++) {
        baseaddr = (i == 0) ? FDC0_BASEADDR : FDC1_BASEADDR;
        fdc_init_controller (&fdc[i], i, FDC0_BASEADDR, dma_floppy_buffer);
    }

    // Done with floppy stuff. No need for IRQ's at this moment
    cli ();
}

/*
    for (i=0; i!=16; i++) kprintf ("%02X ", (Uint8)dma_floppy_buffer[i]);
    kprintf ("\n");

    buffer = malloc
    read_floppy_sectors (0, 0, 10, buffer);

    for (o=0,i=0; i!=10; i++) {
        for (j=0; j!=16; j++, o++) {
            kprintf ("%02X ", (Uint8)dma_floppy_buffer[o]);
        }
        kprintf ("\n");
    }

    for (;;) ;

*/