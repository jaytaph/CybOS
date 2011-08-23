#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fat12.h"

char *image;

/**
 *
 */
void to_cstr (char *src, int size, char *dst) {
    strncpy (dst, src, size);
    dst[size] = '\0';
}

/**
 *
 */
void print_str (char *buf, int size) {
    int i;

    printf ("'");
    for (i=0; i!=size; i++) printf ("%c", *buf++);
    printf ("'\n");
}

/**
 *
 */
void display_bpb () {
    BPB_t *bpb = (BPB_t *)image;

    printf ("Boot\n");
    printf ("  OEMName               : "); print_str(bpb->OEMName, 8);
    printf ("  Bytes per sector      : %d\n", bpb->BytesPerSector);
    printf ("  Sectors per cluster   : %d\n", bpb->SectorsPerCluster);
    printf ("  Reserved sector count : %d\n", bpb->ReservedSectorCount);
    printf ("  Number of fats        : %d\n", bpb->NumberOfFATs);
    printf ("  Max dir entries       : %d\n", bpb->MaxDirEntries);
    printf ("  Total sectors         : %d\n", bpb->TotalSectors);
    printf ("  Media descriptor      : %X\n", bpb->MediaDescriptor);
    printf ("  Sectors per fat       : %d\n", bpb->SectorsPerFAT);
    printf ("  Sectors per track     : %d\n", bpb->SectorsPerTrack);
    printf ("  NUmber of heads       : %d\n", bpb->NumberOfHeads);
    printf ("  Hidden sector count   : %d\n", bpb->HiddenSectorCount);
    printf ("  Total sectors extd    : %d\n", bpb->TotalSectorsExtended);
    printf ("  Drive number          : %d\n", bpb->DriveNumber);
    printf ("  Reserved              : %d\n", bpb->Reserved);
    printf ("  Ext Boot sig          : %X\n", bpb->ExtendedBootSignature);
    printf ("  Serial                : %d\n", bpb->SerialNumber);
    printf ("  Volume label          : "); print_str(bpb->VolumeLabel, 11);
    printf ("  Fat system type       : "); print_str(bpb->FatSystemType, 8);
    printf ("\n\n");
}

/**
 * Returns offset for specified FAT region (fat 1, 2.. etc)
 */
Uint8 *get_fat_region (int FATnumber) {
    BPB_t *bpb = (BPB_t *)image;

    // Sanity check to see if we have givven a correct FAT
    if (FATnumber < 1 || FATnumber > bpb->NumberOfFATs) return NULL;

    // Global info
    Uint32 reservedRegion = 0;
    Uint32 fatRegion = reservedRegion + bpb->ReservedSectorCount;
    Uint32 rootDirectoryRegion = fatRegion + (bpb->NumberOfFATs * bpb->SectorsPerFAT);
    Uint32 dataRegion = rootDirectoryRegion + (bpb->MaxDirEntries * 32 / bpb->BytesPerSector);

    // Calculate FAT start
    Uint32 fat_offset = (fatRegion * bpb->BytesPerSector);

    // Get n'th FAT region
    fat_offset += (FATnumber-1) * bpb->SectorsPerFAT;

    return (Uint8 *) (image + fat_offset);
}

/**
 * Reads the value found on cluster entry. Should be easy, but made difficult because
 * data is stored in 12 bits instead of 16.
 */
Uint16 read_fat_entry (Uint16 cluster_entry) {
    Uint8 *fat = (Uint8 *)get_fat_region (1);

    Uint32 offset = cluster_entry + (cluster_entry / 2);
    Uint16 next_cluster = * ( (Uint16 *)(fat + offset));

    if (cluster_entry % 2 == 0) {
         next_cluster &= 0x0fff;
    } else {
         next_cluster >>= 4;
    }
    return next_cluster;
}



/**
 * Returns byte offset of specified data cluster
 */
void *get_data_cluster (Uint32 cluster) {
    BPB_t *bpb = (BPB_t *)image;


    Uint32 dataCluster = bpb->ReservedSectorCount +
                         (bpb->NumberOfFATs * bpb->SectorsPerFAT) +
                         (bpb->MaxDirEntries * 32 / bpb->BytesPerSector) +
                         ( (cluster-2) * bpb->SectorsPerCluster);
    Uint32 offset = (bpb->BytesPerSector * bpb->SectorsPerCluster * dataCluster);
    return image + offset;
}



/**
 *
 */
void display_fat (int fat_number) {
    BPB_t *bpb = (BPB_t *)image;

    if (fat_number < 1 || fat_number > bpb->NumberOfFATs) {
        printf ("FAT %d does not exists\n");
        return;
    }

    Uint32 fat_offset = bpb->HiddenSectorCount + bpb->ReservedSectorCount;
    fat_offset += (fat_number-1) * bpb->SectorsPerFAT;
    fat_offset *= bpb->BytesPerSector;

    printf ("FAT %d\n", fat_number);
    printf ("  FAT Offset : 0x%X\n", fat_offset);
    printf ("\n");

    int i,j,k;
    for (k=0, i=0; i!=10; i++) {
        printf ("  Entry %04d  : ", i*10);
        for (j=0; j!=10; j++,k++) printf ("%04X ", read_fat_entry (k));
        printf ("\n");
    }
    printf ("\n");
}


/**
 *
 */
void display_root_dir () {
    BPB_t *bpb = (BPB_t *)image;
    DIR_t *entry;
    int i;

    Uint32 entry_offset = bpb->BytesPerSector * (
                          (bpb->HiddenSectorCount + bpb->ReservedSectorCount) +
                          (bpb->NumberOfFATs * bpb->SectorsPerFAT)
                          );
    printf ("ROOT DIR\n");
    printf ("  DIR Offset : 0x%X\n", entry_offset);

    entry = (DIR_t *)(image + entry_offset);
    for (i=0; i!=bpb->MaxDirEntries; i++) {
        if (entry->Filename[0] == 0x00) break;

        if (entry->Attributes != 0xF) {
            Uint32 offset = bpb->BytesPerSector * (
                            bpb->ReservedSectorCount + (bpb->NumberOfFATs * bpb->SectorsPerFAT) +
                            (bpb->MaxDirEntries * 32 / bpb->BytesPerSector) +
                            ((entry->FirstCluster-2) * bpb->SectorsPerCluster));

                char fn[9], ex[4];
                to_cstr(entry->Filename, 8, (char *)&fn);
                to_cstr(entry->Extension, 3, (char *)&ex);

                printf ("  File: '%s.%s' A: %02X  FC: %04X  S: %04X  O: %08X\n", fn, ex, entry->Attributes, entry->FirstCluster, entry->Size, offset);
        }
        entry++;
    }

    printf ("\n");
}


/**
 *
 */
void display_dir (Uint32 firstCluster) {
    BPB_t *bpb = (BPB_t *)image;
    DIR_t *dir;
    int i;

    // Get first cluster
    Uint32 cluster = firstCluster;
    printf ("Directory starts at data cluster %x\n", cluster);
    do {
        Uint32 offset;

        // Read cluster
        for (offset=0, i=0; i!=(bpb->BytesPerSector*bpb->SectorsPerCluster) / 32; i++) {
            char *data = get_data_cluster (cluster);
            DIR_t *entry = (DIR_t *)(data + offset);
//            printf ("ENTRY: %d 0x%X\n", i, entry);
//            printf ("done");

            if (entry->Filename[0] == 0x00) break;

            if (entry->Attributes != 0xF) {
                int offset = bpb->BytesPerSector * (
                bpb->ReservedSectorCount + (bpb->NumberOfFATs * bpb->SectorsPerFAT) +
                (bpb->MaxDirEntries * 32 / bpb->BytesPerSector) +
                ( (entry->FirstCluster-2) * bpb->SectorsPerCluster));

                char fn[9], ex[4];
                to_cstr(entry->Filename, 8, (char *)&fn);
                to_cstr(entry->Extension, 3, (char *)&ex);

                printf ("  File: '%s.%s' A: %02X  FC: %04X  S: %04X  O: %08X\n", fn, ex, entry->Attributes, entry->FirstCluster, entry->Size, offset);
            }

            offset += 32;
        }

        // Position 'cluster' in the FAT points the next cluster
        cluster = read_fat_entry (cluster);
        printf ("Current cluster moved to : %x\n", cluster);
    } while (cluster > 0x002 && cluster < 0xFF7);

    printf ("END\n");
}

/**
 *
 */
void display_file (Uint32 firstCluster) {
    BPB_t *bpb = (BPB_t *)image;
    Uint32 offset;
    int i;

    // Get first cluster
    int cluster = firstCluster;
    printf ("File starts at data cluster %x\n", cluster);
    do {

        Uint8 *data = get_data_cluster (cluster);

        for (offset = 0, i=0; i!=bpb->BytesPerSector*bpb->SectorsPerCluster; i++) {
            if (i && i % 16 == 0) printf ("\n");
            if (i && i % 4 == 0 && i % 16 != 0) printf (" |  ");
            printf ("%02X ", *(data+offset));
            offset++;
        }

        // Position 'cluster' in the FAT points the next cluster
        cluster = read_fat_entry (cluster);
        printf ("Current cluster moved to : %x\n", cluster);
    } while (cluster > 0x002 && cluster < 0xFF7);

    printf ("END\n");
}


/**
 *
 */
int main (int argc, char *argv[]) {
    FILE *f;

    if (argc <= 1) {
        printf ("Usage: fat12 <file>\n\n");
        return 1;
    }

    // Open file and find length
    f = fopen (argv[1], "rb");
    if (f == NULL) {
        printf ("Cannot open file\n\n");
        return 1;
    }
    fseek (f, 0, SEEK_END);
    int filesize = ftell (f);

    // Create buffer for file
    image = (char *)malloc(filesize);
    if (image == NULL) {
        printf ("Cannot malloc() buffer\n\n");
        return 1;
    }

    // Read into buffer
    fseek (f, 0, SEEK_SET);
    int ret = fread (image, 1, filesize, f);
    if (ret != filesize) {
        printf ("Cannot read into buffer\n\n");
        return 1;
    }

    fclose (f);

    // Display boot parameter block
    display_bpb ();

    // Display all FAT's
    BPB_t *bpb = (BPB_t *)image;
    int i;
    for (i=1; i<=bpb->NumberOfFATs; i++) display_fat (i);

    // Display root directory
    display_root_dir ();

    display_dir (3);

    display_file (4);
    display_file (5);

    return 0;
}