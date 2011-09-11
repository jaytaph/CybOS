/******************************************************************************
 *
 *  File        : floppy.h
 *  Description : floppy function defines.
 *
 *****************************************************************************/

#ifndef __DRIVERS_FLOPPY_H__
#define __DRIVERS_FLOPPY_H__

#include "kernel.h"

enum DOR_MASK {
    DOR_MASK_DRIVE0         =   0,  //00000000  = here for completeness sake
    DOR_MASK_DRIVE1         =   1,  //00000001
    DOR_MASK_DRIVE2         =   2,  //00000010
    DOR_MASK_DRIVE3         =   3,  //00000011
    DOR_MASK_RESET          =   4,  //00000100
    DOR_MASK_DMA            =   8,  //00001000
    DOR_MASK_DRIVE0_MOTOR       =   16, //00010000
    DOR_MASK_DRIVE1_MOTOR       =   32, //00100000
    DOR_MASK_DRIVE2_MOTOR       =   64, //01000000
    DOR_MASK_DRIVE3_MOTOR       =   128 //10000000
};

enum MSR_MASK {
    MSR_MASK_DRIVE1_POS_MODE    =   0x01,  //00000001
    MSR_MASK_DRIVE2_POS_MODE    =   0x02,  //00000010
    MSR_MASK_DRIVE3_POS_MODE    =   0x04,  //00000100
    MSR_MASK_DRIVE4_POS_MODE    =   0x08,  //00001000
    MSR_MASK_BUSY               =   0x10,  //00010000
    MSR_MASK_DMA                =   0x20,  //00100000
    MSR_MASK_DATAIO             =   0x40,  //01000000
    MSR_MASK_DATAREG            =   0x80   //10000000
};

// These numbers are relative to FDC's base address (0x3F0 or 0x370)
enum FloppyRegisters {
   FR_STATUS_REGISTER_A                = 0x0, // read-only
   FR_STATUS_REGISTER_B                = 0x1, // read-only
   FR_DIGITAL_OUTPUT_REGISTER          = 0x2,
   FR_TAPE_DRIVE_REGISTER              = 0x3,
   FR_MAIN_STATUS_REGISTER             = 0x4, // read-only
   FR_DATARATE_SELECT_REGISTER         = 0x4, // write-only
   FR_DATA_FIFO                        = 0x5,
   FR_DIGITAL_INPUT_REGISTER           = 0x7, // read-only
   FR_CONFIGURATION_CONTROL_REGISTER   = 0x7  // write-only
};

enum FloppyCommands {
    FC_READ_TRACK             = 2,
    FC_SPECIFY                = 3,
    FC_CHECK_STAT             = 4,
    FC_WRITE_SECT             = 5,
    FC_READ_SECT              = 6,
    FC_CALIBRATE              = 7,
    FC_CHECK_INT              = 8,
    FC_WRITE_DEL_S            = 9,
    FC_READ_ID_S              = 10,
    FC_READ_DEL_S             = 12,
    FC_FORMAT_TRACK           = 13,
    FC_SEEK                   = 15,
    FC_VERSION                = 16
};

#define FDC_SKIP        0x20
#define FDC_DENSITY     0x40
#define FDC_MULTITRACK  0x80

    #define DMA_FLOPPY_CHANNEL 2        // DMA floppy channel is 2

    typedef struct {
        char   *buffer;  // DMA transfer buffer
        Uint16 size;     // size
    } fdc_dma_t;

    // Holds drive info
    typedef struct {
        Uint16 driveinfo;      // driveinfo for this disk
        Uint16 maxCylinder;    // Maximum number of CHS
        Uint16 maxHead;
        Uint16 sectorsPerTrack;
        Uint16 gap3;           // Misc drive data
        Uint16 sectorDTL;
        Uint16 stepRate;
        Uint16 loadTime;
        Uint16 unloadTime;
        Uint16 usePIO;
    } fdc_driveinfo_t;

    // Holds result after a FDC operation (not always all bytes are used)
    typedef struct {
        Uint8 st0;
        Uint8 st1;
        Uint8 st2;
        Uint8 track;
        Uint8 head;
        Uint8 sector;
        Uint8 size;
    } fdc_result_t;

    // global structure
    typedef struct {
        Uint8           drive_num;     // Which drive number is this drive (0 or 1)
        Uint8           drive_type;    // Drive type (as read from CMOS)
        struct fdc      *fdc;          // Backreference to the drive's controller
        Uint8           cur_cylinder;  // Current CHS of drive
        Uint8           cur_head;
        Uint8           cur_sector;
        fdc_driveinfo_t driveinfo;     // driveinfo for this drive

        fdc_result_t result;           // Result of last action
    } fdc_drive_t;

    typedef struct fdc {
        Uint8        controller_num;   // Which controller is this (0 or 1)
        Uint16       base_address;     // Controller's base address (0x3F0 or 0x370)
        Uint8        version;          // Controller version
        fdc_dma_t    dma;
        fdc_drive_t  drives[2];        // Drive info (max 2 drives per controller)

        Uint8        cur_drive;        // Controller is initialized to which drive?
    } fdc_t;

    // Base address for floppy controller 0 and 1
    #define FDC0_BASEADDR 0x03F0
    #define FDC1_BASEADDR 0x0370

    extern char *floppyDMABuffer;

    #define FDC_DMABUFFER_SIZE 512

    // Maximum of 2 controllers (each with 2 drives makes 4 drives max)
    fdc_t fdc[2];

    void fdc_init (void);
    int floppy_interrupt (regs_t *r);
    void fdc_read_floppy_sector (fdc_drive_t *drive, Uint32 lba_sector, char *buffer);

#endif //__DRIVERS_FLOPPY_H__