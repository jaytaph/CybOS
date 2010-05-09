/******************************************************************************
 *
 *  File        : floppy.h
 *  Description : floppy function defines.
 *
 *****************************************************************************/

#ifndef __DRIVERS_FLOPPY_H__
#define __DRIVERS_FLOPPY_H__

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

enum FloppyRegisters
{
   FR_STATUS_REGISTER_A                = 0x3F0, // read-only
   FR_STATUS_REGISTER_B                = 0x3F1, // read-only
   FR_DIGITAL_OUTPUT_REGISTER          = 0x3F2,
   FR_TAPE_DRIVE_REGISTER              = 0x3F3,
   FR_MAIN_STATUS_REGISTER             = 0x3F4, // read-only
   FR_DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
   FR_DATA_FIFO                        = 0x3F5,
   FR_DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
   FR_CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
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

#define SECTOR_DTL_128      0
#define SECTOR_DTL_256      1
#define SECTOR_DTL_512      2
#define SECTOR_DTL_1024     4

#define GAP3_LENGTH_STD    42
#define GAP3_LENGTH_5_14   32
#define GAP3_LENGTH_3_5    27



#define SECTORS_PER_TRACK  80

#define DMA_FLOPPY_CHANNEL 2        // DMA floppy channel is 2

extern char *dma_floppy_buffer;

    void floppy_init (void);

#endif //__DRIVERS_FLOPPY_H__
