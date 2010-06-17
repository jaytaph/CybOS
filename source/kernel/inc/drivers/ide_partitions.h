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

  void ide_read_partition_table (ide_drive_t *drive, Uint32 lba_sector);

#endif //__DRIVERS_IDE_PARTITIONS_H__
