/***
 * Tool to create boot disk.
 * It will do 2 things:
 *
 * Copy the bootlvl1.bin file into the bootsector of the specified FAT12 image.
 * Find bootlvl2.bin file INSIDE the specified FAT12 image and set boot parameters.
 *
 * Usage:
 *  bootalize [imagefile]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libfat12.h"

char *bootsector;           // Points to 512 byte bootsector (loaded from BOOTLVL1.BIN)
char *fat12_image;          // Points to X byte FAT12 image
fat12_diskinfo_t *diskinfo;
fat12_fileinfo_t *fileinfo;

/**
 * On on calling exit ()
 */
void cleanup (void) {
  printf ("\nCleanup()\n");
  if (fat12_image) {
    printf ("Fat12 image cleaned\n");
    free (fat12_image);
  }
  if (bootsector) {
    printf ("Boot sector cleaned\n");
    free (bootsector);
  }

  if (fileinfo) {
    printf ("File info cleaned\n");
    free (fileinfo);
  }
  if (diskinfo) {
    printf ("Disk info cleaned\n");
    free (diskinfo);
  }
}

/**
 *
 */
void copy_bpb (char *fat12_image, char *bootsector) {
    memcpy (bootsector+3, fat12_image+3, 36);
}


/**
 *
 */
void write_bootsector (char *filename, char *bootsector) {
    FILE *f = fopen (filename, "wb");
    if (!f) {
        printf ("Cannot write bootsector onto image\n\n");
        exit (1);
    }

    fwrite (bootsector, 512, 1, f);

    fclose (f);
}


/**
 *
 */
void write_data_in_bootsector (char *filename, long int sector, short int length) {
    // We can open the image file and write the found entries into the correct spot in the
    // boot sector (which happens to be the first entry :p
    FILE *f = fopen (filename, "wb");
    if (!f) {
        printf ("Cannot write data into image bootsector\n\n");
        exit (1);
    }

    fseek (f, 0x3F, SEEK_SET);
    fwrite (&sector, 1, sizeof (long int), f);

    fseek (f, 0x44, SEEK_SET);
    fwrite (&length, 1, sizeof (short int), f);

    fclose (f);
}


int read_boot_file (char *filename, char **buf) {
    FILE *f = fopen (filename, "rb");
    if (f == NULL) return 0;

    // Create buffer for file
    *buf = (char *)malloc(512);
    if (*buf == NULL) return 0;

    // Read into buffer
    fseek (f, 0, SEEK_SET);
    int ret = fread (*buf, 1, 512, f);
    if (ret != 512) return 0;

    fclose (f);
    return 1;
}


/**
 *
 */
int read_file_into_buf (char *filename, char **buf) {
    // Open file and find length
    FILE *f = fopen (filename, "rb");
    if (f == NULL) return 0;

    fseek (f, 0, SEEK_END);
    int filesize = ftell (f);

    // Create buffer for file
    *buf = (char *)malloc (filesize);
    if (*buf == NULL) return 0;

    // Read into buffer
    fseek (f, 0, SEEK_SET);
    int ret = fread (*buf, 1, filesize, f);
    if (ret != filesize) return 0;

    fclose (f);
    return 1;
}

/**
 *
 */
int main (int argc, char *argv[]) {
    char bootfile[256];
    int ret;

    atexit (cleanup);

    if (argc < 2 || argc > 3) {
        printf ("usage: bootalize [imagefile] <bootlvl1.bin>\n\n");
        return 1;
    }

    // Open and read bootfile
    if (argc == 2) {
        strncpy (bootfile, "bootlvl1.bin", 255);
        bootfile[255] = '\0';
    } else {
        strncpy (bootfile, argv[2], 255);
        bootfile[255] = '\0';
    }
    ret = read_boot_file (bootfile, &bootsector);
    if (! ret) {
        printf ("Error reading bootfile '%s'\n\n", bootfile);
        exit (1);
    }

    // Open and read image file
    printf ("ARGV1: %s", argv[1]);
    ret = read_file_into_buf (argv[1], &fat12_image);
    printf ("RET: %d BUF: %08X", ret, fat12_image);
    if (! ret) {
        printf ("Error reading image '%s'\n\n", argv[1]);
        exit (1);
    }

    // Check if image contains valid partition
    diskinfo = fat12_get_disk_info (fat12_image);
    if (! diskinfo) {
        printf ("Image does not look like a valid FAT12 system\n\n");
        exit (1);
    }

    // Check file exists
    char *bootlvl2file = "SYSTEM/BOOTLVL2.BIN";
    fileinfo = fat12_get_file_info (fat12_image, bootlvl2file);
    if (! fileinfo) {
        printf ("Cannot find %s\n\n", bootlvl2file);
        exit (1);
    }

    // Write bootsector into file, but make sure BPB is copied from FAT12 image into bootsector
//    copy_bpb (fat12_image, bootsector);
//    write_bootsector (argv[1], bootsector);

    long int sector = fileinfo->start_cluster;
    short int length = fileinfo->size_bytes;
    printf ("Sector: %d  Size: %d   Name: '%s'  Ext: '%s'\n", sector, length, fileinfo->name, fileinfo->extension);
    write_data_in_bootsector (argv[1], sector, length);

    // All done
    exit (0);
}


