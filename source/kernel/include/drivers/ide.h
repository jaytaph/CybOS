/******************************************************************************
 *
 *  File        : ide.h
 *  Description : ide function defines.
 *
 *  Most code here taken from http://wiki.osdev.org/IDE
 *
 *****************************************************************************/

#ifndef __DRIVERS_IDE_H__
#define __DRIVERS_IDE_H__

    #include "kernel.h"
    #include "pci.h"

    #define IDE_DRIVE_TYPE_ATA        0     // ATA interface needed for this drive
    #define IDE_DRIVE_TYPE_ATAPI      1     // ATAPI interface needed for this drive

    #define IDE_DIRECTION_READ      0x00
    #define IDE_DIRECTION_WRITE     0x01


    // Maximum number of channels per controller
    #define IDE_CONTROLLER_MAX_CHANNELS   4
    #define IDE_CONTROLLER_MAX_DRIVES     2

    #define IDE_DRIVE_0               0
    #define IDE_DRIVE_1               1

    #define IDE_CHANNEL_0             0
    #define IDE_CHANNEL_1             1
    #define IDE_CHANNEL_2             2
    #define IDE_CHANNEL_3             3



    #define IDE_REG_CYL_LO            4
    #define IDE_REG_REG_CYL_HI        5
    #define IDE_REG_REG_DEVSEL        6
    #define IDE_REG_REG_STATUS        7
    #define IDE_REG_REG_COMMAND       7

    #define IDE_REG_DATA          0x00
    #define IDE_REG_ERROR         0x01
    #define IDE_REG_FEATURES      0x01
    #define IDE_REG_SECCOUNT0     0x02
    #define IDE_REG_LBA0          0x03
    #define IDE_REG_LBA1          0x04
    #define IDE_REG_LBA2          0x05
    #define IDE_REG_HDDEVSEL      0x06
    #define IDE_REG_COMMAND       0x07
    #define IDE_REG_STATUS        0x07
    #define IDE_REG_SECCOUNT1     0x08
    #define IDE_REG_LBA3          0x09
    #define IDE_REG_LBA4          0x0A
    #define IDE_REG_LBA5          0x0B
    #define IDE_REG_CONTROL       0x0C
    #define IDE_REG_ALTSTATUS     0x0C
    #define IDE_REG_DEVADDRESS    0x0D

    #define IDE_SR_BSY     0x80
    #define IDE_SR_DRDY    0x40
    #define IDE_SR_DF      0x20
    #define IDE_SR_DSC     0x10
    #define IDE_SR_DRQ     0x08
    #define IDE_SR_CORR    0x04
    #define IDE_SR_IDX     0x02
    #define IDE_SR_ERR     0x01

    // IDE commands
    #define IDE_CMD_READ_PIO          0x20
    #define IDE_CMD_READ_PIO_EXT      0x24
    #define IDE_CMD_READ_DMA          0xC8
    #define IDE_CMD_READ_DMA_EXT      0x25
    #define IDE_CMD_WRITE_PIO         0x30
    #define IDE_CMD_WRITE_PIO_EXT     0x34
    #define IDE_CMD_WRITE_DMA         0xCA
    #define IDE_CMD_WRITE_DMA_EXT     0x35
    #define IDE_CMD_CACHE_FLUSH       0xE7
    #define IDE_CMD_CACHE_FLUSH_EXT   0xEA
    #define IDE_CMD_PACKET            0xA0
    #define IDE_CMD_IDENTIFY_PACKET   0xA1
    #define IDE_CMD_IDENTIFY          0xEC

    // Offsets in identify buffer
    #define IDE_IDENT_DEVICETYPE   0
    #define IDE_IDENT_CYLINDERS    2
    #define IDE_IDENT_HEADS        6
    #define IDE_IDENT_SECTORS      12
    #define IDE_IDENT_SERIAL       20
    #define IDE_IDENT_MODEL        54
    #define IDE_IDENT_CAPABILITIES 98
    #define IDE_IDENT_FIELDVALID   106
    #define IDE_IDENT_MAX_LBA      120
    #define IDE_IDENT_COMMANDSETS  164
    #define IDE_IDENT_MAX_LBA_EXT  200

/*
    // Return values for ide_detect_devtype ()
    #define IDEDEV_NONE          0
    #define IDEDEV_UNKNOWN       1
    #define IDEDEV_PATAPI        2
    #define IDEDEV_SATAPI        3
    #define IDEDEV_PATA          4
    #define IDEDEV_SATA          5
*/


  #define   IDE_SECTOR_SIZE     512     // Size of a sector (@TODO: Atapi CDROM sector sizes?)
  #define   IDE_MAX_PARITIONS   16      // Maximum 16 partitions per drive (because of node numbers)

  // Slave or master drive (either atapi or ata)
  typedef struct ide_drive {
    char                enabled;             // Enabled or not (in case no extra drive on the IDE cable)
    struct ide_channel  *channel;            // Pointer to the channel this drive is on. You can find the
                                             // controller at *drive->channel->controller
    Uint8               drive_nr;            // Master drive (IDE_DRIVE_MASTER) or slave drive (IDE_DRIVE_SLAVE)
    char                type;                // ata (IDE_DRIVE_TYPE_ATAPI) or atapi (IDE_DRIVE_TYPE_ATAPI)
    Uint32              size;                // Size in sectors
    Uint16              signature;
    Uint16              capabilities;
    Uint32              command_sets;
    char                model[41];           // Model of the drive

    char                lba48;               // Device supports LBA48 instead of only LBA28

    char                databuf[IDE_SECTOR_SIZE];    // Simple data structure that holds temporary data for 1 sector at most
  } ide_drive_t;

  // Primary or secondary master channels
  typedef struct ide_channel {
    Uint8                  channel_nr;       // Is this a primary or secondary channel (IDE_CHANNEL_PRIMARY / IDE_CHANNEL_SECONDARY)
    struct ide_controller  *controller;      // Pointer to the controller this channel is on

    Uint16                 base;             // Base port address
    Uint16                 dev_ctl;          // Device port address
    Uint8                  no_int;           // Wheter or not this channel has an IRQ
    Uint16                 bm_ide;           // Bus master IDE

    pci_device_t           *pci;             // Pci device where this controller is on
    struct ide_drive       drive[2];         // Master / slave drive on this channel
  } ide_channel_t;

  /* A complete controller. Each controller can support up to 4 drives. Unlimited controllers could
   * be supported. Allthough there is a MAX_IDE_CONTROLLERS limit in cybos at the moment */
  typedef struct ide_controller {
    char                enabled;             // Needed because we don't use a linked list
    Uint8                controller_nr;
    struct ide_channel  channel[4];          // Channels (everybody tells only 2, but bochs has 4?)
  } ide_controller_t;


  extern char ide_irq_invoked;

  // We support a maximum of 10 IDE controllers. Can change as soon as we use linked lists
  #define MAX_IDE_CONTROLLERS  10
  ide_controller_t ide_controllers[MAX_IDE_CONTROLLERS];  // @TODO : Must be linked list!

  void ide_init (void);

  int ide_polling (ide_channel_t *channel, char advanced_check);
  Uint8 ide_port_read (ide_channel_t *channel, Uint8 reg);
  void ide_port_write (ide_channel_t *channel, Uint8 reg, Uint8 data);

  Uint32 ide_block_read (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
  Uint32 ide_block_write (Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
  void ide_block_open(Uint8 major, Uint8 minor);
  void ide_block_close(Uint8 major, Uint8 minor);
  void ide_block_seek(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction);

  Uint32 ide_sector_read (ide_drive_t *drive, Uint32 lba_sector, Uint32 count, char *buffer);



#endif //__DRIVERS_IDE_H__
