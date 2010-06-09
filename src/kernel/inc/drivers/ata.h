/******************************************************************************
 *
 *  File        : ata.h
 *  Description : ata function defines.
 *
 *****************************************************************************/

#ifndef __DRIVERS_ATA_H__
#define __DRIVERS_ATA_H__

    #include "kernel.h"

    #define REG_CYL_LO  4
    #define REG_CYL_HI  5
    #define REG_DEVSEL  6
    #define REG_STATUS  7
    #define REG_COMMAND 7

    // Return values for ata_detect_devtype ()
    #define ATADEV_NONE          0
    #define ATADEV_UNKNOWN       1
    #define ATADEV_PATAPI        2
    #define ATADEV_SATAPI        3
    #define ATADEV_PATA          4
    #define ATADEV_SATA          5

#define MAX_ATA_CONTROLLERS  2

typedef struct ata_device_drive {
    char    enabled;
    Uint16  identify[256];      // identify information
} ata_device_drive_t;

typedef struct ata_device {
    char    controller_nr;
    Uint16  base;
    Uint16  dev_ctl;
    ata_device_drive_t drive[2];
} ata_device_t;


// max 2 controllers
ata_device_t hdc[MAX_ATA_CONTROLLERS];

    void ata_init (void);

#endif //__DRIVERS_ATA_H__
