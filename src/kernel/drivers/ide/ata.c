/******************************************************************************
 *
 *  File        : ata.c
 *  Description : ATA interface
 *
 *
 *****************************************************************************/

#include "drivers/ide.h"
#include "drivers/ide_ata.h"
#include "kernel.h"
#include "kmem.h"
#include "pci.h"

/**
 *
 */
Uint32 ide_ata_access(char direction, ide_drive_t *drive, Uint32 lba_sector, Uint32 sector_count, char *buf) {
   unsigned char lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
   unsigned char lba_io[6];
   unsigned int slavebit = drive->drive_nr;            // Read the Drive [Master/Slave]
   unsigned int bus = drive->channel->base;            // Bus Base, like 0x1F0 which is also data port.
   unsigned int words = 256;                           // Almost every ATA drive has a sector-size of 512-byte.
   unsigned short cyl, i;
   unsigned char head, sect, err;

  // Disable IRQ's on channel
  ide_port_write (drive->channel, IDE_REG_CONTROL, drive->channel->no_int = (ide_irq_invoked = 0x0) + 0x02);

  // when we want to read sector 0x10000000 or higher, it can only be done with LBA48
  if (lba_sector >= 0x10000000) {
    // lba48
    lba_mode  = 2;
    lba_io[0] = (lba_sector & 0x000000FF) >> 0;
    lba_io[1] = (lba_sector & 0x0000FF00) >> 8;
    lba_io[2] = (lba_sector & 0x00FF0000) >> 16;
    lba_io[3] = (lba_sector & 0xFF000000) >> 24;
    lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
    lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
    head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
  } else if (drive->capabilities & 0x200)  { // Drive supports LBA?
    // lba28:
    lba_mode  = 1;
    lba_io[0] = (lba_sector & 0x00000FF) >> 0;
    lba_io[1] = (lba_sector & 0x000FF00) >> 8;
    lba_io[2] = (lba_sector & 0x0FF0000) >> 16;
    lba_io[3] = 0; // These Registers are not used here.
    lba_io[4] = 0; // These Registers are not used here.
    lba_io[5] = 0; // These Registers are not used here.
    head      = (lba_sector & 0xF000000) >> 24;
  } else {
    // CHS:
    lba_mode  = 0;
    sect      = (lba_sector % 63) + 1;
    cyl       = (lba_sector + 1  - sect) / (16 * 63);
    lba_io[0] = sect;
    lba_io[1] = (cyl >> 0) & 0xFF;
    lba_io[2] = (cyl >> 8) & 0xFF;
    lba_io[3] = 0;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head      = (lba_sector + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
  }


  dma = 0; // We don't support DMA

  while (ide_port_read (drive->channel, IDE_REG_STATUS) & IDE_SR_BSY) ; // Wait if busy.

  if (lba_mode == 0)
    ide_port_write (drive->channel, IDE_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
  else
    ide_port_write (drive->channel, IDE_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA


  // (V) Write Parameters;
  if (lba_mode == 2) {
    ide_port_write (drive->channel, IDE_REG_SECCOUNT1,   0);
    ide_port_write (drive->channel, IDE_REG_LBA3,   lba_io[3]);
    ide_port_write (drive->channel, IDE_REG_LBA4,   lba_io[4]);
    ide_port_write (drive->channel, IDE_REG_LBA5,   lba_io[5]);
  }
  ide_port_write (drive->channel, IDE_REG_SECCOUNT0, sector_count);
  ide_port_write (drive->channel, IDE_REG_LBA0,   lba_io[0]);
  ide_port_write (drive->channel, IDE_REG_LBA1,   lba_io[1]);
  ide_port_write (drive->channel, IDE_REG_LBA2,   lba_io[2]);

  // Select command depending on the lba mode, dma and direction (read/write)
  if (lba_mode == 0 && dma == 0 && direction == 0) cmd = IDE_CMD_READ_PIO;
  if (lba_mode == 1 && dma == 0 && direction == 0) cmd = IDE_CMD_READ_PIO;
  if (lba_mode == 2 && dma == 0 && direction == 0) cmd = IDE_CMD_READ_PIO_EXT;
  if (lba_mode == 0 && dma == 1 && direction == 0) cmd = IDE_CMD_READ_DMA;
  if (lba_mode == 1 && dma == 1 && direction == 0) cmd = IDE_CMD_READ_DMA;
  if (lba_mode == 2 && dma == 1 && direction == 0) cmd = IDE_CMD_READ_DMA_EXT;
  if (lba_mode == 0 && dma == 0 && direction == 1) cmd = IDE_CMD_WRITE_PIO;
  if (lba_mode == 1 && dma == 0 && direction == 1) cmd = IDE_CMD_WRITE_PIO;
  if (lba_mode == 2 && dma == 0 && direction == 1) cmd = IDE_CMD_WRITE_PIO_EXT;
  if (lba_mode == 0 && dma == 1 && direction == 1) cmd = IDE_CMD_WRITE_DMA;
  if (lba_mode == 1 && dma == 1 && direction == 1) cmd = IDE_CMD_WRITE_DMA;
  if (lba_mode == 2 && dma == 1 && direction == 1) cmd = IDE_CMD_WRITE_DMA_EXT;
  ide_port_write (drive->channel, IDE_REG_COMMAND, cmd);

  if (dma) {
    if (direction == 0) {
       // @TODO: DMA Read.
       kpanic ("DMA read is not supported yet.");
    } else {
      // @TODO: DMA Write.
      kpanic ("DMA write is not supported yet.");
    }
  } else {
    if (direction == 0) {
      char *bufptr = buf;
      // PIO Read.
      for (i = 0; i < sector_count; i++) {
        if ( (err = ide_polling (drive->channel, 1)), err != 0) return err; // Polling, set error and exit if there is.
        insw (bus, (Uint32)bufptr, words);
        bufptr += words*2;
      }
    } else {
      char *bufptr = buf;
      // PIO Write.
      for (i = 0; i < sector_count; i++) {
        ide_polling (drive->channel, 0); // Polling.
        outsw (bus, (Uint32)bufptr, words);
        bufptr += words*2;
      }

      // Flush data
      ide_port_write (drive->channel, IDE_REG_COMMAND, (char []) { IDE_CMD_CACHE_FLUSH, IDE_CMD_CACHE_FLUSH, IDE_CMD_CACHE_FLUSH_EXT }[lba_mode]);
      ide_polling (drive->channel, 0); // Polling.
    }
  }

  // Return sector count
  return sector_count;
}