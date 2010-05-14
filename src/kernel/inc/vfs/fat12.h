/******************************************************************************
 *
 *  File        : fat12.h
 *  Description : Fat12 filesystem
 *
 *****************************************************************************/
#ifndef __VFS_FAT12_H__
#define __VFS_FAT12_H__

#pragma pack(1)
typedef struct {
    Uint8   jmp_command[3];

    Uint8   OEMName[8];
    Uint16  BytesPerSector;
    Uint8   SectorsPerCluster;
    Uint16  ReservedSectors;
    Uint8   NumberOfFats;
    Uint16  NumDirEntries;
    Uint16  NumSectors;
    Uint8   Media;
    Uint16  SectorsPerFat;
    Uint16  SectorsPerTrack;
    Uint16  HeadsPerCyl;
    Uint32  HiddenSectors;
    Uint32  LongSectors;

    Uint32  SectorsPerFat32;   //sectors per FAT
    Uint16  Flags;             //flags
    Uint16  Version;           //version
    Uint32  RootCluster;       //starting root directory
    Uint16  InfoCluster;
    Uint16  BackupBoot;        //location of bootsector copy
    Uint16  Reserved[6];
} fat12_bpb_t;

typedef char * fat12_fat_t;

  fs_node_t *fat12_init (int driveNum);

#endif // __VFS_FAT12_H__