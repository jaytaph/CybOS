#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gzip.h"

char *gzip_decode (const char *path) {
  // Open file
  FILE *f = fopen (path, "rb");
  if (!f) return NULL;

  // Find file size
  fseek (f, 0, SEEK_END);
  int gzip_data_size = ftell (f);
  fseek (f, 0, SEEK_SET);

  // Alloc space
  char *gzip_data = (char *)malloc (gzip_data_size);
  if (! gzip_data) goto cleanup;

  // Read file
  int ret = fread (gzip_data, 1, gzip_data_size, f);
  if (ret != gzip_data_size) goto cleanup;

  // Close it
  fclose (f);
  f = NULL;

  // Read block
  char *gzip_data_ptr = gzip_data;

  // Check header
  gzip_header_t *header= gzip_data_ptr;
  if (header->id1 != 0x1F && header->id2 != 0x8B && header->compression_method != 8) goto cleanup;

  // Optional extra field
  if ((header->flags & FLG_FEXTRA) == FLG_FEXTRA) {
    unsigned short *extra_length = gzip_data_ptr;
    printf ("EXTRA_LENGTH: %d\n", extra_length);
    gzip_data_ptr += extra_length;
  }
  // Optional name field
  if ((header->flags & FLG_FNAME) == FLG_FNAME) {
    unsigned char *tmp = gzip_data_ptr;
    printf ("NAME: '%s'\n", tmp);
    gzip_data_ptr += strlen (tmp)+1;
  }
  // Optional comment field
  if ((header->flags & FLG_FCOMMENT) == FLG_FCOMMENT) {
    unsigned char *tmp = gzip_data_ptr;
    printf ("COMMENT: '%s'\n", tmp);
    gzip_data_ptr += strlen (tmp)+1;
  }

  // Optional header CRC field
  if ((header->flags & FLG_FHCRC) == FLG_FHCRC) {
    unsigned short *crc16 = gzip_data_ptr;
    gzip_data_ptr += 2;
  }

  // Now comes the datablocks
  datablock_length = check_data_blocks (&gzip_data_ptr);
  if (! length) goto cleanup;
  unsigned char *datablocks = gzip_data_ptr;
  gzip_data_ptr += datablock_length;

  // CRC32
  unsigned long *crc32 = gzip_data_ptr;
  gzip_data_ptr += 4;

  // Original length
  unsigned long *length = gzip_data_ptr;
  printf ("Decompressing %d bytes : \n", *length);

  // OK
  return gzip_data;

  // Error
cleanup:
  if (f) fclose (f);
  if (gzip_data) free (gzip_data);
  return NULL;
}

char bitptr = 0;
char byteptr = 0;
void set_bit_ptr (unsigned char *ptr, char bitno) {
  byteptr = ptr;
  bitptr = bitno;
}

// Read max 32 bits
char readbits (char bits) {
}

int check_data_blocks (unsigned char *ptr) {
  set_bit_ptr (ptr, 0);

  do {
    unsigned char header = *ptr & 0x7;
    char lastblock = header & 1;
    print "Last block : %d\n", lastblock);
    char compression = (header >> 1) & 0x2;
    print "Compresson: %d\n", compression);

    switch (compression) {
      case 0 :
                // No compression
                unsigned short *len = *ptr;
                ptr+=2;
                unsigned short *nlen = *ptr;
                ptr+=2;

                // Literal data
                ptr+=len;
                break;
      case 1 :
                // Fixed huffman
                break;
      case 2 :
                // Dynamic huffman
                break;
      case 3 :
                return 0;   // ERROR!
    }

  } while (! lastblock);
/*
 do
        read block header from input stream.
               if stored with no compression
                  skip any remaining bits in current partially
                     processed byte
                  read LEN and NLEN (see next section)
                  copy LEN bytes of data to output
               otherwise
                  if compressed with dynamic Huffman codes
                     read representation of code trees (see
                        subsection below)
                  loop (until end of block code recognized)
                     decode literal/length value from input stream
                     if value < 256
                        copy value (literal byte) to output stream
                     otherwise
                        if value = end of block (256)
                           break from loop
                        otherwise (value = 257..285)
                           decode distance from input stream

                           move backwards distance bytes in the output
                           stream, and copy length bytes from this
                           position to the output stream.
                  end loop
            while not last block
*/
}

int main (void) {
  make_crc_table ();

  char *data = gzip_decode ("bochsout.txt.gz");
  return 0;
}