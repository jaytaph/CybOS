/******************************************************************************
 *
 *  File        : ide.c
 *  Description : IDE device
 *
 *  Most code here taken from http://wiki.osdev.org/IDE
 *
 *****************************************************************************/

#include "drivers/ide.h"
#include "drivers/ide_ata.h"
#include "drivers/ide_atapi.h"
#include "drivers/ide_partitions.h"
#include "kernel.h"
#include "kmem.h"
#include "pci.h"

char ide_irq_invoked = 0;

// These are the standard IO ports for the controllers (only 1 controller currently supported)
// @TODO: more controllers could be supported by boot-param: IDE2=x,x,x,x,x,x,x,x
Uint16 ide_controller_ioports[][8] = {
                                      { 0x1F0, 0x3F0, 0x170, 0x370, 0x1E8, 0x3E0, 0x168, 0x360 }
                                     };

/**
 * Write to port
 */
void ide_port_write (ide_channel_t *channel, Uint8 reg, Uint8 data) {
  if (reg > 0x07 && reg < 0x0C) ide_port_write (channel, IDE_REG_CONTROL, 0x80 | channel->no_int);

  if (reg < 0x08) outb (channel->base + reg - 0x00, data);
  else if (reg < 0x0C) outb (channel->base + reg - 0x06, data);
  else if (reg < 0x0E) outb (channel->dev_ctl + reg - 0x0A, data);
  else if (reg < 0x16) outb (channel->bm_ide + reg - 0x0E, data);

  if (reg > 0x07 && reg < 0x0C) ide_port_write (channel, IDE_REG_CONTROL, channel->no_int);
}


/**
 * Read from port
 */
Uint8 ide_port_read (ide_channel_t *channel, Uint8 reg) {
  Uint8 result;

  if (reg > 0x07 && reg < 0x0C) ide_port_write (channel, IDE_REG_CONTROL, 0x80 | channel->no_int);

  if (reg < 0x08) result = inb (channel->base + reg - 0x00);
  else if (reg < 0x0C) result = inb (channel->base + reg - 0x06);
  else if (reg < 0x0E) result = inb (channel->dev_ctl + reg - 0x0A);
  else if (reg < 0x16) result = inb (channel->bm_ide + reg - 0x0E);

  if (reg > 0x07 && reg < 0x0C) ide_port_write(channel, IDE_REG_CONTROL, channel->no_int);
  return result;
}


/**
 *
 */
int ide_polling (ide_channel_t *channel, char advanced_check) {
  int i;

  // 400 uSecond delay by reading the altstatus port 4 times
  for (i=0; i<4; i++) ide_port_read (channel, IDE_REG_ALTSTATUS);

  // Wait for BSY to be zero.
  while (ide_port_read (channel, IDE_REG_STATUS) & IDE_SR_BSY) ;

  if (advanced_check) {
    char state = ide_port_read (channel, IDE_REG_STATUS); // Read Status Register.

    if (state & IDE_SR_DF) return 1;  // Device Fault.
    if (state & IDE_SR_ERR) return 2; // Error.
    if ((state & IDE_SR_DRQ) == 0) return 3; // DRQ should be set
  }

  return 0;
}


/**
 *
 */
void ide_port_read_buffer (ide_channel_t *channel, Uint8 reg, Uint32 buffer, Uint16 quads) {
  if (reg > 0x07 && reg < 0x0C) ide_port_write(channel, IDE_REG_CONTROL, 0x80 | channel->no_int);

  if (reg < 0x08) insl (channel->base  + reg - 0x00, buffer, quads);
  else if (reg < 0x0C) insl (channel->base + reg - 0x06, buffer, quads);
  else if (reg < 0x0E) insl (channel->dev_ctl + reg - 0x0A, buffer, quads);
  else if (reg < 0x16) insl (channel->bm_ide + reg - 0x0E, buffer, quads);

  if (reg > 0x07 && reg < 0x0C) ide_port_write (channel, IDE_REG_CONTROL, channel->no_int);
}





/**
 * Read specified number of sectors from drive into buffer
 */
Uint8 ide_sector_read (ide_drive_t *drive, Uint32 lba_sector, Uint32 count, char *buffer) {
  int ret;

//  kprintf ("ide_sector_read (drive, %d, %d, %08X)\n", lba_sector, count, buffer);

  // Not enabled drive
  if (! drive->enabled) return 0;

  // Incorrect sector
  if (lba_sector > drive->size) return 0;

  if (drive->type == IDE_DRIVE_TYPE_ATA) {
    ret = ide_ata_access (IDE_DIRECTION_READ, drive, lba_sector, count, buffer);
  } else if (drive->type == IDE_DRIVE_TYPE_ATAPI) {
    ret = ide_atapi_access (IDE_DIRECTION_READ, drive, lba_sector, count, buffer);
  } else {
    kpanic ("Unknown type (neither ATA nor ATAPI)");
  }

  return ret;
}


/**
 * Write specified number of sectors from buffer to drive
 */
void ide_write_sectors(ide_drive_t *drive, Uint32 lba_sector, Uint32 count, char *buffer) {
  int ret;

  // Not enabled drive
  if (! drive->enabled) return;

  // Incorrect sector
  if (lba_sector > drive->size) return;

  if (drive->type == IDE_DRIVE_TYPE_ATA) {
    ret = ide_ata_access (IDE_DIRECTION_WRITE, drive, lba_sector, count, buffer);
  } else if (drive->type == IDE_DRIVE_TYPE_ATAPI) {
    ret = ide_atapi_access (IDE_DIRECTION_WRITE, drive, lba_sector, count, buffer);
  } else {
    kpanic ("Unknown type (neither ATA nor ATAPI)");
  }

  return;
}


/**
 * Sleep for X milliseconds. Based on the fact that reading the ALT_STATUS register takes 100 uSeconds
 */
void ide_sleep (ide_channel_t *channel, int ms) {
  int i;
  for (i=0; i<ms*10; i++) ide_port_read (channel, IDE_REG_ALTSTATUS);
}


/**
 *
 */
void ide_init_drive (ide_drive_t *drive) {
  char type = IDE_DRIVE_TYPE_ATA;
  char ide_buffer[512];
  char err = 0;
  char status;
  int i;

//  kprintf ("    ide_init_drive()\n");

  // Default, drive is not enabled
  drive->enabled = 0;

  // Select drive
  ide_port_write (drive->channel, IDE_REG_HDDEVSEL, 0xA0 | (drive->drive_nr << 4));
  ide_sleep (drive->channel, 1);

  ide_port_write (drive->channel, IDE_REG_COMMAND, IDE_CMD_IDENTIFY);
  ide_sleep (drive->channel, 1);

  // Polling:
  if (ide_port_read (drive->channel, IDE_REG_STATUS) == 0) return; // No drive found

  while (1) {
    status = ide_port_read (drive->channel, IDE_REG_STATUS);
    if ((status & IDE_SR_ERR)) { err = 1; break; } // If Err, Device is not ATA.
    if (!(status & IDE_SR_BSY) && (status & IDE_SR_DRQ)) break; // Everything is right.
  }

  // Check if it's an ATAPI drive (if not ATA detected)
  if (err != 0) {
    Uint8 cl = ide_port_read (drive->channel, IDE_REG_LBA1);
    Uint8 ch = ide_port_read (drive->channel, IDE_REG_LBA2);

    if (cl == 0x14 && ch ==0xEB) type = IDE_DRIVE_TYPE_ATAPI;
    else if (cl == 0x69 && ch == 0x96) type = IDE_DRIVE_TYPE_ATAPI;
    else return; // Unknown Type (may not be a device).

    ide_port_write (drive->channel, IDE_REG_COMMAND, IDE_CMD_IDENTIFY_PACKET);
    ide_sleep (drive->channel, 1);
  }

  // Read device identification
  ide_port_read_buffer(drive->channel, IDE_REG_DATA, (Uint32)ide_buffer, 128);

/*
  for (i=0; i!=128; i++) {
    kprintf ("%02X ", (Uint8)ide_buffer[i]);
    if (i % 16 == 15) kprintf ("\n");
  }
*/

  // Read device parameters
  drive->enabled      = 1;
  drive->type         = type;
  drive->signature    = *(Uint16 *)(ide_buffer + IDE_IDENT_DEVICETYPE);
  drive->capabilities = *(Uint16 *)(ide_buffer + IDE_IDENT_CAPABILITIES);
  drive->command_sets = *(Uint32 *)(ide_buffer + IDE_IDENT_COMMANDSETS);

  // Get drive size
  if (drive->command_sets & (1 << 26)) {
    // Device uses 48-Bit Addressing:
    drive->size  = *(Uint32 *)(ide_buffer + IDE_IDENT_MAX_LBA);
    drive->lba48 = 1;
  } else {
    // Device uses CHS or 28-bit Addressing
    drive->size  = *(Uint32 *)(ide_buffer + IDE_IDENT_MAX_LBA);
    drive->lba48 = 0;
  }

//  kprintf ("size %08X sectors\n", drive->size);
//  kprintf ("size %d MB\n", drive->size / 2 / 1024);

  // Get identification string
  for (i=0; i<40; i+=2) {
    drive->model[i] = ide_buffer[IDE_IDENT_MODEL + i + 1];
    drive->model[i+1] = ide_buffer[IDE_IDENT_MODEL + i];
  }
  drive->model[40] = 0; // terminate string



  // Register device so we can access it
  device_t *device = (device_t *)kmalloc (sizeof (device_t));
  device->major_num = DEV_MAJOR_IDE;
  device->minor_num = (drive->channel->controller->controller_nr << 3) + (drive->channel->channel_nr << 1) + drive->drive_nr;

  device->read = ide_block_read;
  device->write = ide_block_write;
  device->open = ide_block_open;
  device->close = ide_block_close;
  device->seek = ide_block_seek;

  // Create device name
  char filename[20];
  memset (filename, 0, sizeof (filename));
  sprintf (filename, "IDE%dC%dD%d", drive->channel->controller->controller_nr, drive->channel->channel_nr, drive->drive_nr);

  // Register device
//  kprintf ("\n*** Registering device DEVICE:/%s\n", filename);
  device_register (device, filename);

  // Initialise partitions from the MBR
  ide_read_partition_table (drive, 0, 0);
//  ide_init_partitions (drive);

//  kprintf ("    ide_init_drive() done \n");
}


/**
 *
 */
void ide_init_channel (ide_channel_t *channel) {
//  kprintf ("  ide_init_channel()\n");

  // Disable IRQ
  ide_port_write (channel, IDE_REG_CONTROL, 2);

  // Init both drives (if any)
  channel->drive[0].channel = channel;
  channel->drive[0].drive_nr = 0;
  ide_init_drive (&channel->drive[0]);

  channel->drive[1].channel = channel;
  channel->drive[1].drive_nr = 1;
  ide_init_drive (&channel->drive[1]);

//  kprintf ("  ide_init_channel() done\n");
}


/**
 *
 */
void ide_init_controller (ide_controller_t *ctrl, pci_device_t *pci_dev, Uint16 io_port[8]) {
//  kprintf ("ide_init_controller(%04X %04X)\n", io_port[0],io_port[1]);

  // Read PCI information for port settings
  Uint32 bar[5];
  bar[0] = pci_config_get_dword (pci_dev, 0x10) & 0xFFFFFFFC;
  bar[1] = pci_config_get_dword (pci_dev, 0x14) & 0xFFFFFFFC;
  bar[2] = pci_config_get_dword (pci_dev, 0x18) & 0xFFFFFFFC;
  bar[3] = pci_config_get_dword (pci_dev, 0x1C) & 0xFFFFFFFC;
  bar[4] = pci_config_get_dword (pci_dev, 0x20) & 0xFFFFFFFC;

  // Set standard info for channel 0 (master)
  int i;
  for (i=0; i!=IDE_CONTROLLER_MAX_CHANNELS; i++) {
    ctrl->channel[i].channel_nr = i;
    ctrl->channel[i].base = io_port[i*2+0];
    ctrl->channel[i].dev_ctl = io_port[i*2+1] + 4;
    ctrl->channel[i].bm_ide = bar[4] + 0;
    ctrl->channel[i].pci = pci_dev;

    ctrl->channel->controller = ctrl;
    ide_init_channel (&ctrl->channel[i]);
  }

  // Enable this controller
  ctrl->enabled = 1;

//  kprintf ("ide_init_controller() done\n");
}


/**
 *
 */
void ide_init (void) {
  int controller_num;

//  kprintf ("ide_init() start\n");

  // Clear ide_controllers info
  memset (ide_controllers, 0, sizeof(ide_controllers));

  // Detect mass controller PCI devices
  pci_device_t *pci_dev = NULL;
  controller_num = 0;
  while (pci_dev = pci_find_next_class (pci_dev, 0x01, 0x01), pci_dev != NULL) {
//    kprintf ("Found controller %04X:%04X [class: %02X:%02X] on [%02x:%02x:%02x]\n", pci_dev->vendor_id, pci_dev->device_id, pci_dev->class, pci_dev->subclass, pci_dev->bus, pci_dev->slot, pci_dev->func);

    // Set controller number (for easy searching)
    ide_controllers[controller_num].controller_nr = controller_num;

    // Initialise controller (and drives)
    ide_init_controller (&ide_controllers[controller_num], pci_dev, ide_controller_ioports[controller_num]);

    // Increase controller number
    controller_num++;
  }

//  kprintf ("ide_init() done\n");



  int i,j,k;
  for (i=0; i!=MAX_IDE_CONTROLLERS; i++) {
    if (! ide_controllers[i].enabled) continue;

//    kprintf ("IDE controller %d\n", i);
    for (j=0; j!=IDE_CONTROLLER_MAX_CHANNELS; j++) {
      for (k=0; k!=IDE_CONTROLLER_MAX_DRIVES; k++) {
        if (! ide_controllers[i].channel[j].drive[k].enabled) continue;

/*
        kprintf ("%02d/%02d: [%s] (%4dMB) '%s'\n",
                 j, k,
                 (ide_controllers[i].channel[j].drive[k].type==0?"ATA  ":"ATAPI"),
                 ide_controllers[i].channel[j].drive[k].size / 1024 / 2,
                 ide_controllers[i].channel[j].drive[k].model
                );
*/
      }
    }
  }

  for (;;);
}




Uint32 ide_block_read (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("ide_block_read(%d, %d, %d, %d, %08X)\n", major, minor, offset, size, buffer);
  kprintf ("read from IDE not supported yet\n");
  return 0;
}
Uint32 ide_block_write (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("ide_block_write(%d, %d, %d, %d, %08X)\n", major, minor, offset, size, buffer);
  kprintf ("write to IDE not supported yet\n");
  return 0;
}
void ide_block_open(Uint8 major, Uint8 minor) {
  kprintf ("ide_block_open(%d, %d)\n", major, minor);
  // Doesn't do anything. Device already open?
}
void ide_block_close(Uint8 major, Uint8 minor) {
  kprintf ("ide_block_close(%d, %d)\n", major, minor);
  // Doesn't do anything. Device never closes?
}
void ide_block_seek(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction) {
  kprintf ("ide_block_seek(%d, %d, %d, %d)\n", major, minor, offset, direction);
  // Doesn't do anything.
}