#ifndef __FAT12_INT_H__
#define __FAT12_INT_H__

    typedef unsigned char           Uint8;
    typedef unsigned short int      Uint16;
    typedef unsigned long int       Uint32;
    typedef unsigned long long      Uint64;

    typedef signed char             Sint8;
    typedef signed short int        Sint16;
    typedef signed long int         Sint32;
    typedef signed long long        Sint64;

    #pragma pack(1)
    typedef struct {
        Uint8    Jump[3];
        Uint8    OEMName[8];
        Uint16   BytesPerSector;
        Uint8    SectorsPerCluster;
        Uint16   ReservedSectorCount;
        Uint8    NumberOfFATs;
        Uint16   MaxDirEntries;
        Uint16   TotalSectors;
        Uint8    MediaDescriptor;
        Uint16   SectorsPerFAT;
        Uint16   SectorsPerTrack;
        Uint16   NumberOfHeads;
        Uint32   HiddenSectorCount;
        Uint32   TotalSectorsExtended;
        Uint8    DriveNumber;
        Uint8    Reserved;
        Uint8    ExtendedBootSignature;
        Uint32   SerialNumber;
        Uint8    VolumeLabel[11];
        Uint8    FatSystemType[8];
    } BPB_t;


    #pragma pack(1)
    typedef struct {
        Uint8    Filename[8];
        Uint8    Extension[3];
        Uint8    Attributes;
        Uint8    Reserved;
        Uint8    CreateTime_fine;
        Uint16   CreateTime_hms;
        Uint16   CreateTime_ymd;
        Uint16   AccessTime_ymd;
        Uint16   EAIndex;
        Uint16   ModifyTime_hms;
        Uint16   ModifyTime_ymd;
        Uint16   FirstCluster;
        Uint32   Size;
    } DIR_t;

#endif // __FAT12_INT_H__