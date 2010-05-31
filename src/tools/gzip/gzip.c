#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Based on the work of -- David Madore <david.madore@ens.fr> 1999/11/21
 */

#include "gzip.h"

const int fht[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

/**
 * Read single bit from bitstream and increases the bit pointer
 */
unsigned char peek_bit_from_bitstream (unsigned char *stream, unsigned int bit_ptr) {
  int byte_ptr = bit_ptr * 8;    // Find correct byte
  int bit_pos = bit_ptr & 0x7;    // Find bit position

  // Get bit
  unsigned char result = ( (unsigned char)(*stream+byte_ptr) >> bit_pos) & 0x1;
//  printf ("RES: %d (%d)\n", result, bit_ptr);
  return result;
}


/**
 *
 */
unsigned int peek_bitstream (unsigned char *stream, unsigned int bit_ptr, int bits) {
  unsigned int ret = 0;
  int i;

  for (i=0; i!=bits; i++) ret += (int)peek_bit_from_bitstream (stream, bit_ptr+i) << i;
  return ret;
}


/**
 * Read a serie of bits from the bitstream
 */
unsigned int read_bitstream (unsigned char *stream, unsigned int *bit_ptr, int bits) {
  unsigned int ret = 0;
  int i;

  for (i=0; i!=bits; i++, (*bit_ptr)++) ret += (int)peek_bit_from_bitstream (stream, *bit_ptr) << i;
  return ret;
}


/**
 * 'Deflates' uncompressed data (copies N bytes from input to output). No compression is used
 */
int deflate_uncompressed (unsigned char *in, unsigned int *in_ptr, unsigned char *out, unsigned int *out_ptr) {
  int i;

  // Sync to next byte
  while ( (*in_ptr & 0x7) != 0) in_ptr++;

  unsigned short len = read_bitstream (in, in_ptr, 16);
  unsigned short nlen = read_bitstream (in, in_ptr, 16);

  // Check if len and nlen is correct
  if (len + nlen != 65535) return 0;

  // Move in bytes to out buffer
  for (i=0; i!=len; i++) {
    unsigned char c = read_bitstream (in, in_ptr, 8);
    *(out+(*out_ptr++)) = c;  // @TODO: nice output function?
  }

  return 1;
}


/**
 *
 */
void make_code_table (huffman_t *tree) {
  printf ("void make_code_table ()\n");

  unsigned int code = 0;
  int i,j;

  for (i=1; i<=tree->maxbits; i++) {
    for (j=0; j<tree->length; j++) {
      if (tree->size_table[j] == i) tree->code_table[j] = code++;
    }
    code <<= 1;
  }
}


/**
 * This creates a fixed huffman tree (standarized tree). Saves room since it's well, standard, so
 * it does not has to be added to the deflate data.
 */
void createFixedHuffmanTables (huffman_t *hlit_tree, huffman_t *hdist_tree) {
  unsigned int code = 0;
  int i,j;

  // Init literals
  for (i=0; i<144; i++) hlit_tree->size_table[i] = 8;
  for ( ; i<256; i++) hlit_tree->size_table[i] = 9;
  for ( ; i<280; i++) hlit_tree->size_table[i] = 7;
  for ( ; i<288; i++ ) hlit_tree->size_table[i] = 8;
  hlit_tree->maxbits = 15;
  hlit_tree->length = 288;
  make_code_table (hlit_tree);

  // Init distances
  for (i=0; i<30; i++ ) hdist_tree->size_table[i] = 5;
  hdist_tree->maxbits = 15;
  hdist_tree->length = 30;
  make_code_table (hdist_tree);
}



/**
 *
 */
int createDynamicHuffmanTables (huffman_t *hlit_tree, huffman_t *hdist_tree, unsigned char *in, unsigned int *in_ptr) {
  huffman_t codetree;     // Initial tree used to compress the actual tree.. (hmzz)
  int i,j,k;
  int remainder, rem_val;
  char b;

  char hlit = read_bitstream (in, in_ptr, 5);
  char hdist = read_bitstream (in, in_ptr, 5);
  char hclen = read_bitstream (in, in_ptr, 4);

  printf ("hlit: %d\n", hlit);
  printf ("hdist: %d\n", hdist);
  printf ("hclen: %d\n", hclen);

  // Init code tree
  codetree.maxbits = 15;
  codetree.length = 19;
  for (i=0; i<4+hclen; i++) codetree.size_table[fht[i]] = read_bitstream (in, in_ptr, 3);
  for ( ; i<codetree.length ; i++) codetree.size_table[fht[i]] = 0;
  make_code_table (&codetree);


printf ("HLIT table\n");

  remainder = 0;
  rem_val = 0;
  for (j=0; j<257+hlit; j++) {
    printf ("J: %d\n", j);
    b = decode_one (codetree, in, in_ptr);
    printf ("B: %d\n", b);
    if (b<16) {
      hlit_tree->size_table[j] = b;
    } else if (b == 16) {
      k = read_bitstream (in, in_ptr, 2);
      for (i=0; i<k+3 &&  j+i<257+hlit; i++) hlit_tree->size_table[j+i] = hlit_tree->size_table[j-1];
      j += i-1;
      remainder = k+3-i;
      rem_val = hlit_tree->size_table[j-1];
    } else if (b == 17) {
      k = read_bitstream (in, in_ptr, 3);
      for (i=0; i<k+3 && j+i<257+hlit; i++) hlit_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+3-i;
      rem_val = 0;
    } else if (b == 18) {
      k = read_bitstream (in, in_ptr, 7);
      for (i=0; i<k+11 && j+i<267+hlit; i++) hlit_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+11-i;
      rem_val = 0;
    } else {
      // Huh?
    }
    for ( ; j < hlit_tree->length; j++) hlit_tree->size_table[j] = 0;
    make_code_table (hlit_tree);
  }


  for (j=0; j<remainder; j++) hdist_tree->size_table[j] = rem_val;
  for ( ; j < 1+hdist; j++) {
    b = decode_one (codetree, in, in_ptr);
    if (b<16) {
      hdist_tree->size_table[j] = b;
    } else if (b == 16) {
      k = read_bitstream (in, in_ptr, 2);
      for (i=0; i<k+3 &&  j+i<257+hlit; i++) hdist_tree->size_table[j+i] = hdist_tree->size_table[j-1];
      j += i-1;
      remainder = k+3-i;
      rem_val = hdist_tree->size_table[j-1];
    } else if (b == 17) {
      k = read_bitstream (in, in_ptr, 3);
      for (i=0; i<k+3 && j+i<257+hlit; i++) hdist_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+3-i;
      rem_val = 0;
    } else if (b == 18) {
      k = read_bitstream (in, in_ptr, 7);
      for (i=0; i<k+11 && j+i<267+hlit; i++) hdist_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+11-i;
      rem_val = 0;
    } else {
      // Huh?
    }
    for ( ; j < hdist_tree->length; j++) hdist_tree->size_table[j] = 0;
    make_code_table (hdist_tree);
  }
}


/**
 *
 */
int decode_one (unsigned char *in, unsigned int *bit_ptr, huffman_t *tree) {
  unsigned int code;
  int i,j;

  printf ("decode_one");

  // Read a 'number' of bits
  code = 0;
  for (i=0; i < tree->maxbits; i++) {
    code = (code << 1) + peek_bitstream (in, *bit_ptr+i, 1);
  }

  for (j=0; j < tree->length; j++) {
    if (tree->size_table[j] && ( (code>>(tree->maxbits-tree->size_table[j])) == tree->code_table[j]) ) {
      *bit_ptr += tree->size_table[j];
      return j;
    }
  }

  // Something went wrong :(
  printf ("decode_one(): incorrect bits");
  return -1;
}


/**
 * Deflate data according to the corresponding huffman tree
 */
int deflate_data (huffman_t *hlit_tree, huffman_t *hdist_tree, unsigned char *in, unsigned int *bit_ptr, unsigned char *out, unsigned int *out_ptr) {
  unsigned int b;

  while (1) {
    b = decode_one (in, bit_ptr, hlit_tree);
    // Literal?
    if (b < 256) {
      *(out+(*out_ptr++)) = b;  // Literal char, dump it
    }
    // End of block?
    if (b == 256) return 1;     // Nicely done

    // Back reference (LZ77?)
    if (b >= 257) {
      unsigned int length,dist;
      unsigned int l;

      // @TODO: A lookup table would be nicer...
      switch (b) {
        case 257: length = 3; break;
        case 258: length = 4; break;
        case 259: length = 5; break;
        case 260: length = 6; break;
        case 261: length = 7; break;
        case 262: length = 8; break;
        case 263: length = 9; break;
        case 264: length = 10; break;
        case 265: length = 11 + read_bitstream (in, bit_ptr, 1); break;
        case 266: length = 13 + read_bitstream (in, bit_ptr, 1); break;
        case 267: length = 15 + read_bitstream (in, bit_ptr, 1); break;
        case 268: length = 17 + read_bitstream (in, bit_ptr, 1); break;
        case 269: length = 19 + read_bitstream (in, bit_ptr, 2); break;
        case 270: length = 23 + read_bitstream (in, bit_ptr, 2); break;
        case 271: length = 27 + read_bitstream (in, bit_ptr, 2); break;
        case 272: length = 31 + read_bitstream (in, bit_ptr, 2); break;
        case 273: length = 35 + read_bitstream (in, bit_ptr, 3); break;
        case 274: length = 43 + read_bitstream (in, bit_ptr, 3); break;
        case 275: length = 51 + read_bitstream (in, bit_ptr, 3); break;
        case 276: length = 59 + read_bitstream (in, bit_ptr, 3); break;
        case 277: length = 67 + read_bitstream (in, bit_ptr, 4); break;
        case 278: length = 83 + read_bitstream (in, bit_ptr, 4); break;
        case 279: length = 99 + read_bitstream (in, bit_ptr, 4); break;
        case 280: length = 115 + read_bitstream (in, bit_ptr, 4); break;
        case 281: length = 131 + read_bitstream (in, bit_ptr, 5); break;
        case 282: length = 163 + read_bitstream (in, bit_ptr, 5); break;
        case 283: length = 195 + read_bitstream (in, bit_ptr, 5); break;
        case 284: length = 227 + read_bitstream (in, bit_ptr, 5); break;
        case 285: length = 258; break;
        default : return 0;
      }

      unsigned int bb = decode_one (in, bit_ptr, hdist_tree);
      // @TODO: This too is all fixed data, we should use a lookup buffer (easy peasy)
      switch (bb) {
        case 0: dist = 1; break;
        case 1: dist = 2; break;
        case 2: dist = 3; break;
        case 3: dist = 4; break;
        case 4: dist = 5 + read_bitstream (in, bit_ptr, 1); break;
        case 5: dist = 7 + read_bitstream (in, bit_ptr, 1); break;
        case 6: dist = 9 + read_bitstream (in, bit_ptr, 2); break;
        case 7: dist = 13 + read_bitstream (in, bit_ptr, 2); break;
        case 8: dist = 17 + read_bitstream (in, bit_ptr, 3); break;
        case 9: dist = 25 + read_bitstream (in, bit_ptr, 3); break;
        case 10: dist = 33 + read_bitstream (in, bit_ptr, 4); break;
        case 11: dist = 49 + read_bitstream (in, bit_ptr, 4); break;
        case 12: dist = 65 + read_bitstream (in, bit_ptr, 5); break;
        case 13: dist = 97 + read_bitstream (in, bit_ptr, 5); break;
        case 14: dist = 129 + read_bitstream (in, bit_ptr, 6); break;
        case 15: dist = 193 + read_bitstream (in, bit_ptr, 6); break;
        case 16: dist = 257 + read_bitstream (in, bit_ptr, 7); break;
        case 17: dist = 385 + read_bitstream (in, bit_ptr, 7); break;
        case 18: dist = 513 + read_bitstream (in, bit_ptr, 8); break;
        case 19: dist = 769 + read_bitstream (in, bit_ptr, 8); break;
        case 20: dist = 1025 + read_bitstream (in, bit_ptr, 9); break;
        case 21: dist = 1537 + read_bitstream (in, bit_ptr, 9); break;
        case 22: dist = 2049 + read_bitstream (in, bit_ptr, 10); break;
        case 23: dist = 3073 + read_bitstream (in, bit_ptr, 10); break;
        case 24: dist = 4097 + read_bitstream (in, bit_ptr, 11); break;
        case 25: dist = 6145 + read_bitstream (in, bit_ptr, 11); break;
        case 26: dist = 8193 + read_bitstream (in, bit_ptr, 12); break;
        case 27: dist = 12289 + read_bitstream (in, bit_ptr, 12); break;
        case 28: dist = 16385 + read_bitstream (in, bit_ptr, 13); break;
        case 29: dist = 24577 + read_bitstream (in, bit_ptr, 13); break;
        default : return 0;
      }

      // Copy the string of 'length' size which is 'dist' bytes back in the OUTPUT buffer
      int i;
      for (i=0 ; i<length; i++ ) {
        unsigned char ch = *(out+(*out_ptr - dist));    // Get byte 'dist' positions away
        *(out+(*out_ptr++)) = ch;                       // place it at the end
      }
    }
  }
}


/**
 *
 */
int deflate (unsigned char *in, unsigned char *out) {
  printf ("deflate()\n");

  huffman_t hlit_tree, hdist_tree;
  unsigned int bit_ptr = 0;
  char bfinal = 0;
  unsigned int out_ptr = 0;

  do {
    bfinal = read_bitstream (in, &bit_ptr, 1);
    printf ("bfinal: %d\n", bfinal);
    char btype = read_bitstream (in, &bit_ptr, 2);

    printf ("btype: %d\n", btype);
    switch (btype) {
      case 0  :
                // No encoding
                if (!deflate_uncompressed (in, &bit_ptr, out, &out_ptr)) return 0;
                break;
      case 1  :
                // Fixed huffman
                createFixedHuffmanTables (&hlit_tree, &hdist_tree);
                if (! deflate_data (&hlit_tree, &hdist_tree, in, &bit_ptr, out, &out_ptr)) return 0;
                break;
      case 2  :
                // Dynamic huffman
                createDynamicHuffmanTables (&hlit_tree, &hdist_tree, in, &bit_ptr);
                if (! deflate_data (&hlit_tree, &hdist_tree, in, &bit_ptr, out, &out_ptr)) return 0;
                break;
      case 3  :
                // Reserved
      default :
                return 0;
                break;
    }
  } while (!bfinal);
}


/**
 *
 */
unsigned char *gzip_decode (unsigned char *buf, int buf_len) {
  printf ("gzip_decode(%d)\n", buf_len);
  unsigned char *buf_ptr = buf;
  gzip_t data;

  // Init our gzip data structure
  memset (&data, 0, sizeof (data));

  // Check header
  gzip_header_t *header = (gzip_header_t *)buf_ptr;
  buf_ptr += sizeof (gzip_header_t);
  data.header = header;

  // Check header signature & compression method
  if (header->id1 != 0x1F && header->id2 != 0x8B && header->compression_method != 8) return NULL;

  // Optional extra field
  if ((header->flags & FLG_FEXTRA) == FLG_FEXTRA) {
    unsigned short *extra_length = (unsigned short *)buf_ptr;
    printf ("EXTRA_LENGTH: %d\n", extra_length);
    buf_ptr += 2;
    buf_ptr += *extra_length;
    // TODO: do extra
  }
  // Optional name field
  if ((header->flags & FLG_FNAME) == FLG_FNAME) {
    unsigned char *tmp = buf_ptr;
    data.filename = buf_ptr;
    printf ("NAME: '%s'\n", tmp);
    buf_ptr += strlen (tmp)+1;
  }
  // Optional comment field
  if ((header->flags & FLG_FCOMMENT) == FLG_FCOMMENT) {
    unsigned char *tmp = buf_ptr;
    data.comment = buf_ptr;
    printf ("COMMENT: '%s'\n", tmp);
    buf_ptr += strlen (tmp)+1;
  }

  // Optional header CRC field
  if ((header->flags & FLG_FHCRC) == FLG_FHCRC) {
    unsigned short *crc16 = (unsigned short *)buf_ptr;
    data.crc16 = *crc16;
    buf_ptr += 2;
  }

  data.data = buf_ptr;

  // Set buffer pointer to end of buffer (@TODO: read deflateblocks first)
  buf_ptr = (buf + buf_len - 4 - 4);

  // CRC32
  unsigned long *crc32 = (unsigned long *)buf_ptr;
  data.crc32 = *crc32;
  buf_ptr += 4;

  // Original length
  unsigned long *length = (unsigned long *)buf_ptr;
  data.size = *length;
  printf ("Decompressing %d bytes : \n", *length);


  printf ("Data.header.id1           : %02x\n", data.header->id1);
  printf ("Data.header.id2           : %02x\n", data.header->id2);
  printf ("Data.header.cm            : %02x\n", data.header->compression_method);
  printf ("Data.header.flags         : %02x\n", data.header->flags);
  printf ("Data.header.mtime         : %d\n", data.header->mtime);
  printf ("Data.header.extra_flags   : %02x\n", data.header->extra_flags);
  printf ("Data.extra                : %08X\n", data.extra);
  printf ("Data.filename             : %08X (%s)\n", data.filename, data.filename ? (char *)data.filename : "NULL");
  printf ("Data.comment              : %08X (%s)\n", data.comment, data.comment ? (char *)data.comment : "NULL");
  printf ("Data.text                 : %08X (%s)\n", data.text, data.text ? (char *)data.text : "NULL");
  printf ("Data.crc16                : %08X\n", data.crc16);
  printf ("Data.crc32                : %08X\n", data.crc32);
  printf ("Data.size                 : %08X\n", data.size);

  printf ("Deflating %s (%d bytes) : ", data.filename, data.size);

  // Allocate buffer and deflate
  unsigned char *outbuf = (unsigned char *)malloc (data.size);
  if (! deflate (data.data, outbuf)) {
    printf ("Error while deflating\n");
    return NULL;
  }

  printf ("\n");

  // OK
  return (unsigned char *)outbuf;
}


/**
 *
 */
int main (void) {
  printf ("main(void)\n");

  // Open file
  FILE *f = fopen ("bochsout.txt.gz", "rb");
  if (!f) return 0;

  // Find file size
  fseek (f, 0, SEEK_END);
  int buf_len = ftell (f);
  fseek (f, 0, SEEK_SET);

  // Alloc space
  char *buf = (char *)malloc (buf_len);
  if (! buf) return 0;

  // Read file
  int ret = fread (buf, 1, buf_len, f);
  if (ret != buf_len) return 0;

  // Decode buffer
  unsigned char *data = gzip_decode (buf, buf_len);
  return 0;
}