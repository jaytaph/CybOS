/******************************************************************************
 *
 *  File        : ata.c
 *  Description : ATA device
 *
 *
 *****************************************************************************/

#include "drivers/ata.h"
#include "kernel.h"
#include "kmem.h"



/**
 */
void ata_soft_reset (Uint16 ctrl_port) {
  kprintf ("soft reset\n");

  outb (ctrl_port, 1 << 4);

  // Wait a while until reset happened
  inb (ctrl_port);
  inb (ctrl_port);
  inb (ctrl_port);
  inb (ctrl_port);

  // Wait until we got BSY=0 && RDY=1
  char status;
  do {
    status = inb (ctrl_port);
  } while ( (status & 0xC0) == 0x40);

  kprintf ("soft reset done\n");
}


int ata_identify_drive (int slave, ata_device_t *ctrl) {
  int i;

  // Default, drive not enabled
  ctrl->drive[slave].enabled = 0;

  // Check floating bus
  Uint16 st = inb (ctrl->base + REG_STATUS);
  kprintf ("DETECTE: %02x\n", st);
  if (st == 0xFF) return 0;


//  ata_soft_reset(ctrl->dev_ctl);
  outb(ctrl->base + REG_DEVSEL, 0xA0 | slave<<4);

  // wait 400ns for drive select to work
  inb(ctrl->base);
  inb(ctrl->base);
  inb(ctrl->base);
  inb(ctrl->base);

  outb(ctrl->base + REG_COMMAND, 0xEC);
  // wait 400ns for drive select to work
  inb(ctrl->base + REG_STATUS);
  inb(ctrl->base + REG_STATUS);
  inb(ctrl->base + REG_STATUS);
  inb(ctrl->base + REG_STATUS);

  // Read initial status
  st = inb(ctrl->base + REG_STATUS);
  kprintf ("STATUS FROM DEV_CTL: %02x\n", st);
  if (st == 0) return 0;

  // Wait until error or ready
  do {
    st = inb(ctrl->base + REG_STATUS);
  } while ( (st & 0xC0) == 0x80 || (st & 1) == 1);

  // Read identify data
  for (i=0; i!=256; i++) {
      ctrl->drive[slave].identify[i] = inw (ctrl->base);
  }

  kprintf ("\n\n");
  kprintf ("LBA28: %04X\n", ctrl->drive[slave].identify[60]);
  kprintf ("LBA28: %04X\n", ctrl->drive[slave].identify[61]);

  kprintf ("LBA48: %04X\n", ctrl->drive[slave].identify[100]);
  kprintf ("LBA48: %04X\n", ctrl->drive[slave].identify[101]);
  kprintf ("LBA48: %04X\n", ctrl->drive[slave].identify[102]);
  kprintf ("LBA48: %04X\n", ctrl->drive[slave].identify[103]);

  kprintf ("LBA48 is %ssupported\n", (ctrl->drive[slave].identify[83]&1<<10)?"":"not ");

  ctrl->drive[slave].enabled = 1;



  // Register device so we can access it
  device_t *device = (device_t *)kmalloc (sizeof (device_t));
  device->majorNum = DEV_MAJOR_HDC;
  device->minorNum = (ctrl->controller_nr * 2) + slave;

//  device->read = hdc_block_read;
//  device->write = hdc_block_write;
//  device->open = hdc_block_open;
//  device->close = hdc_block_close;
//  device->seek = hdc_block_seek;

  // Create device name
  char filename[12];
  memset (filename, 0, sizeof (filename));
  sprintf (filename, "HDC%dD%d", ctrl->controller_nr, device->minorNum);

  // Register device
  kprintf ("\n*** Registering device DEVICE:/%s\n", filename);
  device_register (device, filename);
  return 1;
}

/**
 *
 */
void ata_controller_init (ata_device_t *ctrl) {
  kprintf ("ata_controller_init()\n");
  ata_identify_drive (0, ctrl);     // Identify master
  ata_identify_drive (1, ctrl);     // Identify slave
}

/**
 */
void ata_init (void) {
  kprintf("ata_init()\n");

  // Clear hdc info
  memset (hdc, 0, sizeof(hdc));

  // Set first and second controller information (@TODO: read from PCI)
  hdc[0].controller_nr = 0; hdc[0].base = 0x1F0; hdc[0].dev_ctl = 0x3F6;
  hdc[1].controller_nr = 1; hdc[1].base = 0x170; hdc[1].dev_ctl = 0x376;

  // Init controllers
  ata_controller_init (&hdc[0]);
  ata_controller_init (&hdc[1]);

  for (;;);
}