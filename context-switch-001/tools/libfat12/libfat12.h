#ifndef __LIBFAT12_H__
#define __LIBFAT12_H__

  typedef unsigned char           Uint8;
  typedef unsigned short int      Uint16;
  typedef unsigned long int       Uint32;
  typedef unsigned long long      Uint64;

  typedef signed char             Sint8;
  typedef signed short int        Sint16;
  typedef signed long int         Sint32;
  typedef signed long long        Sint64;

    typedef struct fat12_diskinfo {
        Uint8    OEMName[9];
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
        Uint8    VolumeLabel[12];
        Uint8    FatSystemType[9];
    } fat12_diskinfo_t;

    typedef struct fat12_fileinfo {
        Uint8    name[9];
        Uint8    extension[4];
        Uint8    attributes;

        Uint32   start_bytes;
        Uint32   start_cluster;

        Uint32   size_sectors;
        Uint32   size_bytes;
    } fat12_fileinfo_t;


    fat12_diskinfo_t *fat12_get_disk_info (char *image);
    fat12_fileinfo_t *fat12_get_file_info (char *image, char *filename);

#endif // __LIBFAT12_H__