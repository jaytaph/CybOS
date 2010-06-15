/******************************************************************************
 *
 *  File        : partitions.c
 *  Description : Partioning for ide devices
 *
 *
 *****************************************************************************/

#include "drivers/ide.h"
#include "drivers/ide_partitions.h"
#include "kernel.h"
#include "kmem.h"
#include "pci.h"


/**
 * Read parition table from specified drive and sector. Checks for signature when
 * reading from sector 0.
 *
 * Restrictions:
 *   - Will only read 16 partitions
 *   - Extended partitions will only go 1 level deep (no extended partition into extended partitions)
 */
void ide_read_partition_table (ide_drive_t *drive, Uint32 lba_sector, int base_partition) {
  char buffer[512];
  int i;
  struct ide_mbr *mbr = (struct ide_mbr *)buffer;

//  kprintf ("Reading partition table on sector %08X\n", lba_sector);

  ide_sector_read (drive, lba_sector, 1, buffer);
  if (lba_sector == 0 && (buffer[510] != 0x55 && buffer[511] != 0xAA)) return;   // No 55AA magic found at end of MBR

  for (i=0; i!=4; i++) {
    if (mbr->partition[i].system_id == 0) continue;

//    kprintf ("Checking partition %d from base_partition %d (%02X)\n", i, base_partition, mbr->partition[i].system_id);

//    kprintf ("Partition info:\n");
//    kprintf ("boot : %02X\n", mbr->partition[i].boot);
//    kprintf ("id   : %02X\n", mbr->partition[i].system_id);
//    kprintf ("lba  : %d\n", mbr->partition[i].first_lba_sector);
//    kprintf ("size : %d\n", mbr->partition[i].size);
//    kprintf ("end  : %d\n", mbr->partition[i].first_lba_sector + mbr->partition[i].size);
//    kprintf ("\n");



    // Is this an extended partition?
    if (mbr->partition[i].system_id == 0x05 || mbr->partition[i].system_id == 0x0F) {
      if (lba_sector != 0) kprintf ("Cannot read extended partition inside extended partition\n");

      // Read extended table (recursive if needed)
      ide_read_partition_table (drive, lba_sector + mbr->partition[i].first_lba_sector, i+1);
      continue;
    }

    // Register device so we can access it
    device_t *device = (device_t *)kmalloc (sizeof (device_t));
    device->major_num = DEV_MAJOR_HDC;
    device->minor_num = (drive->channel->controller->controller_nr << 3) + (drive->channel->channel_nr << 1) + drive->drive_nr;
    device->minor_num *= 16;
    device->minor_num += (base_partition * 4) + i + 1;

    device->read  = ide_block_read;
    device->write = ide_block_write;
    device->open  = ide_block_open;
    device->close = ide_block_close;
    device->seek  = ide_block_seek;

//    kprintf ("  Controller: %d\n", drive->channel->controller->controller_nr);
//    kprintf ("  Channel: %d\n", drive->channel->channel_nr);
//    kprintf ("  Drive: %d\n", drive->drive_nr);
//    kprintf ("  Partition: %d\n", (base_partition * 4) + i  + 1);

    // Create device name
    char filename[20];
    memset (filename, 0, sizeof (filename));
    sprintf (filename, "IDE%dC%dD%dP%d", drive->channel->controller->controller_nr, drive->channel->channel_nr, drive->drive_nr, (base_partition * 4) + i + 1);

    // Register device
//    kprintf ("\n*** Registering device DEVICE:/%s\n", filename);
    device_register (device, filename);
  }
}