/******************************************************************************
 *
 *  File        : partitions.c
 *  Description : Partitioning for ide devices
 *
 *
 *****************************************************************************/

#include "drivers/ide.h"
#include "drivers/ide_partitions.h"
#include "device.h"
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
static int ide_partitions_count = 0;
void ide_read_partition_table (ide_drive_t *drive, Uint32 lba_sector) {
  char buffer[512];
  int i;

  /* Reset partition count to zero when we are browsing the MBR (sector 0). We could have entered
   * this function through an extended partition, in which case we do not need to reset the count. */
  if (lba_sector == 0) ide_partitions_count = 0;

//  kprintf ("Reading partition table on sector %08X\n", lba_sector);

  ide_sector_read (drive, lba_sector, 1, buffer);
  if (lba_sector == 0 && (buffer[510] != 0x55 && buffer[511] != 0xAA)) return;   // No 55AA magic found

  // MBR points to buffer
  struct ide_mbr *mbr = (struct ide_mbr *)buffer;

  for (i=0; i!=4; i++) {
    if (mbr->partition[i].system_id == 0) continue;

/*
    kprintf ("P%d: boot : %02X  id: %02X   lba: %08d   size: %08d   end: %08d\n",
            i,
            mbr->partition[i].boot,
            mbr->partition[i].system_id,
            mbr->partition[i].first_lba_sector,
            mbr->partition[i].size,
            mbr->partition[i].first_lba_sector + mbr->partition[i].size);
*/

    // Register device so we can access it
    device_t *device = (device_t *)kmalloc (sizeof (device_t));
    device->major_num = DEV_MAJOR_HDC;
    device->minor_num = (drive->channel->controller->controller_nr << 3) + (drive->channel->channel_nr << 1) + drive->drive_nr;
    device->minor_num *= 16;
    device->minor_num += ide_partitions_count;

    // Sortakinda proxy functions that take the partition offset into account
    device->read  = ide_partition_block_read;
    device->write = ide_partition_block_write;
    device->open  = ide_partition_block_open;
    device->close = ide_partition_block_close;
    device->seek  = ide_partition_block_seek;

    ide_partition_t *partition = (ide_partition_t *)kmalloc(sizeof(ide_partition_t));
    partition->drive = drive;
    partition->bootable = mbr->partition[i].boot;
    partition->lba_start = lba_sector + mbr->partition[i].first_lba_sector;
    partition->lba_size = mbr->partition[i].size;
    partition->lba_end = partition->lba_start + partition->lba_size;
    partition->system_id = mbr->partition[i].system_id;

    device->data = (ide_partition_t *)partition;

    // Create device name
    char filename[20];
    memset (filename, 0, sizeof (filename));
    sprintf (filename, "IDE%dC%dD%dP%d", drive->channel->controller->controller_nr, drive->channel->channel_nr, drive->drive_nr, ide_partitions_count);

    // Register device
//    kprintf ("\n*** Registering device DEVICE:/%s\n", filename);
    device_register (device, filename);

    // Next partition count
    ide_partitions_count++;


    // Is this an extended partition?
    if (mbr->partition[i].system_id == 0x05 || mbr->partition[i].system_id == 0x0F) {
      // if (lba_sector != 0) kprintf ("Cannot read extended partition inside extended partition\n");

      // Read extended table (recursive if needed)
      ide_read_partition_table (drive, lba_sector + mbr->partition[i].first_lba_sector);
    }
  }
}



Uint32 ide_partition_block_read (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer) {
  if (major != DEV_MAJOR_HDC) return 0;

//  kprintf("ide_partition_block_read(%08X (%04X)\n", offset, size);

  device_t *device = device_get_device(major, minor);
  if (! device) return 0;

  ide_partition_t *partition = (ide_partition_t *)device->data;

  // We can (must) calculate the minor IDE node
  Uint8 ide_minor = minor / 16;

/*
  Layout minor node for IDE partitions: Cccd pppp
     C = controller (0 or 1)
     cc = channel (0-3)
     d = drive (0 or 1)
     pppp = partition (0-16)
*/


  // if size > offset + size of disk, trunk size
  if (offset + size > (partition->lba_size * IDE_SECTOR_SIZE)) {
    kprintf ("Trunking size since we are out of partition bounds");
    size = (partition->lba_size * IDE_SECTOR_SIZE) - offset;
  }

  // if offset > size of disk return 0
  if (offset > (partition->lba_end * IDE_SECTOR_SIZE)) {
    kprintf ("Trying to read outside partition bounds");
    return 0;
  }

  Uint32 partition_offset = offset + (partition->lba_start * IDE_SECTOR_SIZE);

//  kprintf ("partition offset: %d\n", partition_offset);

  return ide_block_read(DEV_MAJOR_IDE, ide_minor, partition_offset, size, buffer);
}

Uint32 ide_partition_block_write (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer) {
  if (major != DEV_MAJOR_HDC) return 0;
  kprintf ("ide_partition_block_write(%d, %d, %d, %d, %08X)\n", major, minor, offset, size, buffer);
  kprintf ("write to IDE not supported yet\n");
  return 0;
}
void ide_partition_block_open(Uint8 major, Uint8 minor) {
  if (major != DEV_MAJOR_HDC) return;
  kprintf ("ide_partition_block_open(%d, %d)\n", major, minor);
  // Doesn't do anything. Device already open?
}
void ide_partition_block_close(Uint8 major, Uint8 minor) {
  if (major != DEV_MAJOR_HDC) return;
  kprintf ("ide_partition_block_close(%d, %d)\n", major, minor);
  // Doesn't do anything. Device never closes?
}
void ide_partition_block_seek(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction) {
  if (major != DEV_MAJOR_HDC) return;
  kprintf ("ide_partition_block_seek(%d, %d, %d, %d)\n", major, minor, offset, direction);
  // Doesn't do anything.
}