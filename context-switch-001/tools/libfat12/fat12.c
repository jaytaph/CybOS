/*************************************************************************************
 *
 *  FAT12 API
 *
 *
 *************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fat12.h"          // External API header
#include "fat12_int.h"      // Internal FAT12 structures

/**
 *
 */
fat12_diskinfo_t *fat12_get_disk_info (char *image) {
    BPB_t *bpb = (BPB_t *)image;

    fat12_diskinfo_t *di = (fat12_diskinfo_t *)malloc (sizeof (fat12_diskinfo_t));
    if (di == NULL) return di;

    strncpy (di->OEMName, bpb->OEMName, 8);
    di->OEMName[8] = '\0';

    strncpy (di->VolumeLabel, bpb->VolumeLabel, 11);
    di->VolumeLabel[11] = '\0';

    strncpy (di->FatSystemType, bpb->FatSystemType, 8);
    di->FatSystemType[8] = '\0';

    di->BytesPerSector = bpb->BytesPerSector;
    di->SectorsPerCluster = bpb->SectorsPerCluster;
    di->ReservedSectorCount = bpb->ReservedSectorCount;
    di->NumberOfFATs = bpb->NumberOfFATs;
    di->MaxDirEntries = bpb->MaxDirEntries;
    di->TotalSectors = bpb->TotalSectors;
    di->MediaDescriptor = bpb->MediaDescriptor;
    di->SectorsPerFAT = bpb->SectorsPerFAT;
    di->SectorsPerTrack = bpb->SectorsPerTrack;
    di->NumberOfHeads = bpb->NumberOfHeads;
    di->HiddenSectorCount = bpb->HiddenSectorCount;
    di->TotalSectorsExtended = bpb->TotalSectorsExtended;
    di->DriveNumber = bpb->DriveNumber;
    di->ExtendedBootSignature = bpb->ExtendedBootSignature;
    di->SerialNumber = bpb->SerialNumber;

    return di;
}

/**
 *
 */
fat12_fileinfo_t *fat12_get_file_info (char *image, char *filename) {
    BPB_t *bpb = (BPB_t *)image;

    fat12_fileinfo_t *fi = (fat12_fileinfo_t *)malloc (sizeof (fat12_fileinfo_t));
    if (fi == NULL) return fi;

    strncpy (fi->name, dir->Filename, 8);
    fi->name[8] = '\0';

    strncpy (fi->extension, dir->Extension, 3);
    fi->extension[3] = '\0';

    fi->attributes = di->attribytes;

    // TODO: Check if this is correct ?
    fi->start_bytes = (entry->firstCluster - 2) * bpb->SectorsPerCluster +
                      (bpb->NumberOfFATs * bpb->SectorsPerFAT) +
                      (bpb->HiddenSectorCount + bpb->ReservedSectorCount) +
                      (bpb->MaxDirEntries * 32 / bpb->BytesPerSector);
    fi->start_cluster = entry->firstCluster;
    fi->size_sectors = ceil(entry->size / bpb->SectorsPerCluster / bpb->BytesPerSector);
    fi->size_bytes = entry->size;

    return fi;
}
