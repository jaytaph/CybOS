/******************************************************************************
 *
 *  File        : ide_ata.h
 *  Description : ATA interface
 *
 *****************************************************************************/

#ifndef __DRIVERS_IDE_ATA_H__
#define __DRIVERS_IDE_ATA_H__

  #include "drivers/ide.h"

  Uint32 ide_ata_access(char direction, ide_drive_t *drive, Uint32 lba_sector, Uint32 sector_count, char *buf);

#endif //__DRIVERS_IDE_ATA_H__
