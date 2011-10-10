/******************************************************************************
 *
 *  File        : ide_paritions.h
 *  Description : ide partioning function defines.
 *
 *****************************************************************************/

#ifndef __DRIVERS_IDE_PARTITIONS_H__
#define __DRIVERS_IDE_PARTITIONS_H__

  #include "drivers/ide.h"

 #pragma pack(1)
  struct ide_mbr_part {
    Uint8   boot;
    Uint8   first_head;
    Uint8   first_sector;
    Uint8   first_cylinder;
    Uint8   system_id;
    Uint8   last_head;
    Uint8   last_sector;
    Uint8   last_cylinder;
    Uint32  first_lba_sector;
    Uint32  size;
  };

  #pragma pack(1)
  struct ide_mbr {
    Uint8 code[446];
    struct ide_mbr_part partition[4];
    Uint16 signature;
  };
  
  typedef struct {
      ide_drive_t  *drive;        // IDE drive structure for this partition
      Uint32       lba_start;     // LBA start
      Uint32       lba_end;       // LBA end
      Uint32       lba_size;      // start - end
      Uint8        bootable;      // 0 = not bootable
      Uint8        system_id;     // 83 = linux
  } ide_partition_t;

  void ide_read_partition_table (ide_drive_t *drive, Uint32 lba_sector);
  
  Uint32 ide_partition_block_read (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
  Uint32 ide_partition_block_write (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
  void ide_partition_block_open(Uint8 major, Uint8 minor);
  void ide_partition_block_close(Uint8 major, Uint8 minor);
  void ide_partition_block_seek(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction);

#endif //__DRIVERS_IDE_PARTITIONS_H__
