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

static volatile Uint8 receivedIRQ = 0;

char *dma_floppy_buffer;


/**
 * Waits and returns 1 when FDC is ready or 0 when not ready on time
 */
int wait_status_ready () {
    int i, status;

    for (i=0; i!=500; i++) {
        status = inb (FR_MAIN_STATUS_REGISTER);
        if (status & MSR_MASK_DATAREG) return 1;
    }
    return 0;
}


/**
 *
 */
void wait_floppy_irq (void) {
    kprintf ("WaitFloppyIRQ: ");

    // Wait until it's not 0
    while (!receivedIRQ) ;

    kprintf ("received\n");

    // Reset to 0
    receivedIRQ = 0;
}


/**
 *
 */
void send_command (Uint8 command) {
    if (! wait_status_ready()) {
        kprintf ("Floppy send error");
        kdeadlock ();
    }

    kprintf ("SC: %02X\n", command);
    return outb (FR_DATA_FIFO, command);
}


/**
 * Receives data (or deadlocks)
 */
Uint8 recv_data (void) {
    if (! wait_status_ready()) {
        kprintf ("Floppy recv error");
        kdeadlock ();
    }

    Uint8 data = inb (FR_DATA_FIFO);
    kprintf ("RD: %02X\n", data);
    return data;
}


/**
 * Floppy interrupt handler. Does nothing except setting flag
 */
void floppy_interrupt (regs_t *r) {
    receivedIRQ = 1;
}


/**
 * Detect floppy disk
 */
void detect_floppy_disks (void) {
    Uint8 floppyType, i;

    // Read CMOS for number of floppy disks
    outb (0x70, 0x10);
    for (i=0; i!=100; i++) ;     // Delay a bit
    floppyType = inb (0x71);

    kprintf ("\n");
    kprintf ("Floppy disk types: %02X\n", floppyType);
    kprintf ("Detected master floppy: %s\n", floppyDescriptions[((floppyType >> 4) & 0x7)]);
    kprintf ("Detected slave floppy: %s\n", floppyDescriptions[(floppyType & 0x7)]);
}


/**
 * Check interrupt status of FDC
 */
void check_interrupt_status (Uint32 *statusRegister, Uint32 *currentCylinder) {
    send_command (FC_CHECK_INT);
    *statusRegister = recv_data ();
    *currentCylinder = recv_data ();
}


/**
 * Enabled or disables specified drive motor
 */
void control_motor (Uint8 drive, Uint8 start) {
    // Drive must be 0-3
    if (drive > 3) return;

    if (start) {
        // Doable because all drive bits are sequential
        Uint8 motor = (DOR_MASK_DRIVE0_MOTOR << drive);
        outb (FR_DIGITAL_OUTPUT_REGISTER, drive | motor | DOR_MASK_RESET | DOR_MASK_DMA);
    } else {
        outb (FR_DIGITAL_OUTPUT_REGISTER, drive | DOR_MASK_RESET | DOR_MASK_DMA);
    }

    // sleep a bit
    int i;
    for (i=0; i!=200000; i++) ;
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
void set_drive_data (Uint32 step_rate, Uint32 load_time, Uint32 unload_time, Uint8 use_pio) {
	Uint32 data = 0;
	send_command (FC_SPECIFY);

	data = ( (step_rate & 0xf) << 4) | (unload_time & 0xf);
	send_command (data);

	data = (load_time) << 1 | use_pio;
	kprintf ("Data: %02X\n", data);
    send_command (data);
}

/**
 *
 */
int calibrate_floppy_drive (Uint8 drive) {
    Uint32 st0, cyl;
    int i;

    if (drive >= 4) return 0;

    // Set motor
    control_motor (drive, 1);

    // Do a few times
    for (i=0; i<10; i++) {
        send_command (FC_CALIBRATE);
        send_command (drive);
        wait_floppy_irq ();
        check_interrupt_status (&st0, &cyl);

        // The drive is still on cylinder 0, so return
        if (!cyl) {
            // Shut off motor
            control_motor (drive, 0);
            return 1;
        }
    }

    // Tried a few times, but floppy drive is still not on cylinder 0, return error
    control_motor (drive, 0);
    return 0;
}

/**
 * Resets the floppy controller
 */
void reset_floppy_controller (void) {
    Uint32 st, cyl;
    int i;

    kprintf ("Resetting floppy controler: ");

    // Disable controller
    outb (FR_DIGITAL_OUTPUT_REGISTER, 0x00);

    // Enable controller
    outb (FR_DIGITAL_OUTPUT_REGISTER, DOR_MASK_DRIVE0 | DOR_MASK_RESET | DOR_MASK_DMA);

    // Wait for IRQ to happen
    wait_floppy_irq ();

    // Sense interrupt status (4 times, why? mechanical delay reason?)
    for (i=0; i!=4; i++) check_interrupt_status (&st, &cyl);

    // Set controller to 500Kpbs
    outb (FR_CONFIGURATION_CONTROL_REGISTER, 0x00); // Set to 500 Kbps

    // We should set this data depending on the drive type we detected  (1.44MB etc)
    set_drive_data (3, 16, 240, 0);
    calibrate_floppy_drive (0);

    kprintf ("done\n");
}




/**
 * Return floppy controller version
 */
void get_floppy_controller_version (void) {
    kprintf ("Floppy controller: ");

    send_command (FC_VERSION);
    Uint8 v = recv_data ();
    switch ( v ) {
      case 0xFF: kprintf(" no controller found\n");
                 break;
      case 0x80: kprintf(" NEC controller\n");
                 break;
      case 0x81: kprintf(" VMware controller\n");
                 break;
      case 0x90: kprintf(" enhanced controller\n");
                 break;
      default:   kprintf(" unknown controller [%d]\n", v);
                 break;
    }
}


/**
 * Converts LBA sector to CHS format
 */
void convert_LBA_to_CHS (Uint32 lba_sector, Uint32 *cylinder, Uint32 *head, Uint32 *sector) {
   *cylinder = lba_sector / ( SECTORS_PER_TRACK * 2 );
   *head = ( lba_sector % ( SECTORS_PER_TRACK * 2 ) ) / ( SECTORS_PER_TRACK );
   *sector = lba_sector % SECTORS_PER_TRACK + 1;
}

/**
 * Reads CHS data
 */
void read_floppy_sector_CHS (Uint8 drive, Uint32 cylinder, Uint32 head, Uint32 sector) {
	Uint32 st0, cyl;
	int i;

	kprintf ("READ_FLOPPY_SECTOR_CHS\n");

	//! read in a sector
	send_command (FC_READ_SECT | FDC_MULTITRACK | FDC_SKIP | FDC_DENSITY);
	send_command (head << 2 | drive);
	send_command (cylinder);
	send_command (head);
	send_command (sector);
	send_command (SECTOR_DTL_512 );
	send_command ( ( ( sector + 1 ) >= SECTORS_PER_TRACK ) ? SECTORS_PER_TRACK : sector + 1 );
	send_command (GAP3_LENGTH_3_5 );
	send_command (0xff );

	// Wait for IRQ to happen (ie: sector read by DMA)
	wait_floppy_irq ();

	// Read status information (but discard it)
	for (i=0; i<7; i++) recv_data ();
	check_interrupt_status (&st0,&cyl);
}


/**
 * Seek floppy track
 */
int seek_floppy_track (Uint8 drive, Uint32 cylinder, Uint32 head) {
    Uint32 st0, cyl;
    int i;

    kprintf ("seek_floppy_track (%d, %d, %d)", drive, cylinder, head);
    for (i=0; i!=10; i++) {
        send_command (FC_SEEK);
        send_command ( (head) << 2 | drive);
        send_command (cylinder);

        wait_floppy_irq ();
        check_interrupt_status (&st0, &cyl);
        kprintf ("ST0: %02X CYL: %02X\n", st0, cyl);
        if (cyl == cylinder) return 1;      // We are at the correct cylinder
    }

    // Not (yet) there..
    return 0;
}


/**
 * Reads sector from drive
 */
void read_floppy_sector (Uint8 drive, Uint32 lba_sector) {
    Uint32 c,h,s;

    // Make sure we have a correct drive
    if (drive >= 4) return;

    kprintf ("Read floppy sector %d:%08X\n", drive, lba_sector);

    // Convert LBA sector to CHS
    convert_LBA_to_CHS (lba_sector, &c, &h, &s);

    kprintf ("C: %d  H: %d   S: %d\n", c, h, s);

    // Start motor
    control_motor (drive, 1);

    // Seek correct cylinder (and head)
    while (! seek_floppy_track (drive, c, h)) ;

    // Read CHS sector
    read_floppy_sector_CHS (drive, h, c, s);

    // Shut down motor
    control_motor (drive, 0);

    return;
}


/**
 *
 */
void floppy_init (void) {
    int size = 512;
    int i;

    // Needed since floppy init depends on IRQ
    sti ();


    // @TODO:  Allocate floppy buffer by using the "normal" kmalloc (or dma_kmalloc or something)
    // Initialize DMA buffer for channel 2 (floppy disk)
    dma_reset ();
    dma_mask_channel (DMA_FLOPPY_CHANNEL);

    // Set buffer address
    dma_reset_flipflop (1);
    dma_set_address (DMA_FLOPPY_CHANNEL, LO8((int)&dma_floppy_buffer), HI8((int)&dma_floppy_buffer));

    // Set buffer size
    dma_reset_flipflop (1);
    dma_set_count (DMA_FLOPPY_CHANNEL, LO8(size), HI8(size));

    // Init for reading
    dma_set_read (DMA_FLOPPY_CHANNEL);
    dma_unmask_all ();

    detect_floppy_disks ();
    reset_floppy_controller();
    get_floppy_controller_version();

BOCHS_BREAKPOINT
    for (i=0; i!=16; i++) dma_floppy_buffer[i] = 0x90;

    read_floppy_sector (0, 0);

BOCHS_BREAKPOINT
//    dma_floppy_buffer += 0xC0000000;

    for (i=0; i!=16; i++) kprintf ("%02x ", dma_floppy_buffer[i]);

    for (;;) ;

    // Not needed anymore for now
    cli ();
}