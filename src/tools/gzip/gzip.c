#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/**
 * Based on the work of -- David Madore <david.madore@ens.fr> 1999/11/21
 *
 * See http://www.w3.org/Graphics/PNG/RFC-1951 for more information about the deflate
 */

#include "gzip.h"

// This is the order in which the dynamic tree is compressed. Live with it...
const int fht[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

/* Instead of big switch() statements, these 2 tables hold the byte, it's corresponding
 * length and number of bits that additionally must be read and added to the length.
 */
const deflate_table_t deflate_table_lengths[] = {
  { 257,   3, 0 }, { 258,   4, 0 }, { 259,   5, 0 }, { 260,   6, 0 },
  { 261,   7, 0 }, { 262,   8, 0 }, { 263,   9, 0 }, { 264,  10, 0 },
  { 265,  11, 1 }, { 266,  13, 1 }, { 267,  15, 1 }, { 268,  17, 1 },
  { 269,  19, 2 }, { 270,  23, 2 }, { 271,  27, 2 }, { 272,  31, 2 },
  { 273,  35, 3 }, { 274,  43, 3 }, { 275,  51, 3 }, { 276,  59, 3 },
  { 277,  67, 4 }, { 278,  83, 4 }, { 279,  99, 4 }, { 280, 115, 4 },
  { 281, 131, 5 }, { 282, 163, 5 }, { 283, 195, 5 }, { 284, 227, 5 },
  { 285, 258, 0 }, {   0,   0, 0 }
};

const deflate_table_t deflate_table_dists[] = {
  {  0,     1,  0 }, {  1,     2,  0 }, {  2,     3,  0 }, {  3,     4,  0 },
  {  4,     5,  1 }, {  5,     7,  1 }, {  6,     9,  2 }, {  7,    13,  2 },
  {  8,    17,  3 }, {  9,    25,  3 }, { 10,    33,  4 }, { 11,    49,  4 },
  { 12,    65,  5 }, { 13,    97,  5 }, { 14,   129,  6 }, { 15,   193,  6 },
  { 16,   257,  7 }, { 17,   385,  7 }, { 18,   513,  8 }, { 19,   769,  8 },
  { 20,  1025,  9 }, { 21,  1537,  9 }, { 22,  2049, 10 }, { 23,  3073, 10 },
  { 24,  4097, 11 }, { 25,  6145, 11 }, { 26,  8193, 12 }, { 27, 12289, 12 },
  { 28, 16385, 13 }, { 29, 24577, 13 }, {  0 ,    0,  0 }
};

/**
 * Peeks a number of bits from the stream. Use 'ofset' to change start position to peek
 */
unsigned int peek_input (io_t *io, int bits, int offset) {
  unsigned int ret = 0;
  int i;

  // Peek all needed bits
  for (i=0; i!=bits; i++) {
    int byte_pos = (io->in_ptr + i + offset) / 8;    // Find correct byte to peek
    int bit_pos = (io->in_ptr + i + offset) & 0x7;   // Find bit position in byte

    unsigned char b = (io->in[byte_pos] >> bit_pos) & 0x1;
    ret += b << i;
  }
  return ret;
}


/**
 * Read a serie of bits from the bitstream and increases the bit pointer
 */
unsigned int read_input (io_t *io, int bits) {
  unsigned int ret = peek_input (io, bits, 0);
  io->in_ptr += bits;
  return ret;
}


/**
 * peeks a BYTE from the output string (distance bytes away from the current position)
 */
unsigned char peek_output (io_t *io, int distance) {
  return io->out[io->out_length - distance];
}


/**
 * Pushes a BYTE to the output streem
 */
void write_output (io_t *io, unsigned char b) {
  io->out[io->out_length] = b;
  io->out_length++;

  // If we are at the end of our allocated buffer, increase buffer with 1K
  if (io->out_length == io->out_size) {
    io->out_size += 1024;
    printf ("Resize to %d\n", io->out_size);
    io->out = realloc (io->out, io->out_size);

  }
}


/**
 * Decodes one byte according to the huffman tree given
 */
int decode_one (io_t *io, huffman_t *tree) {
  unsigned int code = 0;
  int i,j;

  // Read a 'number' of bits
  for (i=0; i<tree->maxbits; i++) {
    code = (code << 1) + peek_input (io, 1, i);
  }


  for (j=0; j<tree->length; j++) {
    if (tree->size_table[j] && ( (code >> (tree->maxbits-tree->size_table[j])) == tree->code_table[j]) ) {
      io->in_ptr += tree->size_table[j];
      return j;
    }
  }

  // Something went wrong :(
  return -1;
}


/**
 *
 */
void make_code_table (huffman_t *tree, int maxbits, int length) {
  unsigned int code = 0;
  int i,j;

  //Set bits and length
  tree->maxbits = maxbits;
  tree->length = length;

  // Fill code table
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

  // http://www.w3.org/Graphics/PNG/RFC-1951#fixed for more information about these fixed numbers

  // Init literals
  for (i=0; i<144; i++) hlit_tree->size_table[i] = 8;
  for ( ; i<256; i++) hlit_tree->size_table[i] = 9;
  for ( ; i<280; i++) hlit_tree->size_table[i] = 7;
  for ( ; i<288; i++ ) hlit_tree->size_table[i] = 8;
  make_code_table (hlit_tree, HLIT_MAXBITS, HLIT_TSIZE);

  // Init distances
  for (i=0; i<30; i++ ) hdist_tree->size_table[i] = 5;       // All distances are 5 bits
  make_code_table (hdist_tree, HDIST_MAXBITS, HDIST_TSIZE);
}


/**
 *
 */
int createDynamicHuffmanTables (huffman_t *hlit_tree, huffman_t *hdist_tree, io_t *io) {
  huffman_t codetree;     // Initial tree used to compress the actual tree.. (hmzz)
  int i,j,k;
  int remainder, rem_val;
  char b;

  char hlit = read_input (io, 5);
  char hdist = read_input (io, 5);
  char hclen = read_input (io, 4);

  // Init code tree
  for (i=0; i<4+hclen; i++) codetree.size_table[fht[i]] = read_input (io, 3);
  for ( ; i < 19 ; i++) codetree.size_table[fht[i]] = 0; // (fill with 0's if not completely present)
  make_code_table (&codetree, CLEN_MAXBITS, CLEN_TSIZE);


  // Decompress literals table
  remainder = 0;
  rem_val = 0;
  for (j=0; j<257+hlit; j++) {
    b = decode_one (io, &codetree);
    if (b == -1) return 0;    // Error while decoding

    if (b<16) {
      hlit_tree->size_table[j] = b;
    } else if (b == 16) {
      k = read_input (io, 2);
      for (i=0; i<k+3 &&  j+i<257+hlit; i++) hlit_tree->size_table[j+i] = hlit_tree->size_table[j-1];
      j += i-1;
      remainder = k+3-i;
      rem_val = hlit_tree->size_table[j-1];
    } else if (b == 17) {
      k = read_input (io, 3);
      for (i=0; i<k+3 && j+i<257+hlit; i++) hlit_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+3-i;
      rem_val = 0;
    } else if (b == 18) {
      k = read_input (io, 7);
      for (i=0; i<k+11 && j+i<267+hlit; i++) hlit_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+11-i;
      rem_val = 0;
    } else {
      // Huh?
    }
  }
  for ( ; j < HLIT_TSIZE; j++) hlit_tree->size_table[j] = 0; // Pad with 0's
  make_code_table (hlit_tree, HLIT_MAXBITS, HLIT_TSIZE);



  // Decompress distance table
  for (j=0; j<remainder; j++) hdist_tree->size_table[j] = rem_val;
  for ( ; j < 1+hdist; j++) {
    b = decode_one (io, &codetree);
    if (b == -1) return 0; // Error while decoding
    if (b<16) {
      hdist_tree->size_table[j] = b;
    } else if (b == 16) {
      k = read_input (io, 2);
      for (i=0; i<k+3 &&  j+i<257+hlit; i++) hdist_tree->size_table[j+i] = hdist_tree->size_table[j-1];
      j += i-1;
      remainder = k+3-i;
      rem_val = hdist_tree->size_table[j-1];
    } else if (b == 17) {
      k = read_input (io, 3);
      for (i=0; i<k+3 && j+i<257+hlit; i++) hdist_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+3-i;
      rem_val = 0;
    } else if (b == 18) {
      k = read_input (io, 7);
      for (i=0; i<k+11 && j+i<267+hlit; i++) hdist_tree->size_table[j+i] = 0;
      j += i-1;
      remainder = k+11-i;
      rem_val = 0;
    } else {
      // Huh?
    }
  }
  for ( ; j < HDIST_TSIZE; j++) hdist_tree->size_table[j] = 0; // Padd with 0's
  make_code_table (hdist_tree, HDIST_MAXBITS, HDIST_TSIZE);
}



/**
 * 'Deflates' uncompressed data (copies N bytes from input to output). No compression is used
 */
int deflate_uncompressed (io_t *io) {
  int i;

  // Sync input bit pointer to next byte
  while ( (io->in_ptr & 0x7) != 0) io->in_ptr++;

  unsigned short len = read_input (io, 16);
  unsigned short nlen = read_input (io, 16);

  // Check if len and nlen is correct
  if (len + nlen != 65535) return 0;

  // Move in bytes to out buffer
  for (i=0; i!=len; i++) {
    unsigned char c = read_input (io, 8);
    write_output (io, c);
  }

  return 1;
}



/**
 * Deflate data according to the corresponding huffman tree
 */
int deflate_huffman (huffman_t *hlit_tree, huffman_t *hdist_tree, io_t *io) {
  unsigned int b, i;

  // Repeat until done
  while (1) {
    // Get byte
    b = decode_one (io, hlit_tree);
    if (b == -1) return 0;    // Error while decoding

    // Literal?
    if (b < 256) write_output (io, b);

    // End of block?
    if (b == 256) return 1;     // Nicely done decoding, return

    // Back reference (LZ77?)
    if (b >= 257) {
      unsigned int length,dist;
      unsigned int l;

      // Read length data from lookup table
      i=0;
      do {
        if (deflate_table_lengths[i].code == b) {
          length = deflate_table_lengths[i].length;
          if (deflate_table_lengths[i].readbits > 0) length += read_input (io, deflate_table_lengths[i].readbits);
          break;
        }
        i++;
      } while (deflate_table_lengths[i].length != 0);

      // Read distance
      unsigned int bb = decode_one (io, hdist_tree);

      // Read distance data from lookup table
      i=0;
      do {
        if (deflate_table_dists[i].code == bb) {
          dist = deflate_table_dists[i].length;
          if (deflate_table_dists[i].readbits > 0) dist += read_input (io, deflate_table_dists[i].readbits);
          break;
        }
        i++;
      } while (deflate_table_dists[i].length != 0);

      // Copy the string of 'length' size which is 'dist' bytes back in the OUTPUT buffer
      int i;
      for (i=0 ; i<length; i++ ) {
        unsigned char ch = peek_output (io, dist);
        write_output (io, ch);
      }
    }
  }
}


/**
 *
 */
int deflate (io_t *io) {
  huffman_t hlit_tree, hdist_tree;
  char bfinal = 0;

  do {
    bfinal = read_input (io, 1);
    char btype = read_input (io, 2);

    switch (btype) {
      case 0  :
                // No encoding
                if (!deflate_uncompressed (io)) return 0;
                break;
      case 1  :
                // Fixed huffman
                createFixedHuffmanTables (&hlit_tree, &hdist_tree);
                if (! deflate_huffman (&hlit_tree, &hdist_tree, io)) return 0;
                break;
      case 2  :
                // Dynamic huffman
                createDynamicHuffmanTables (&hlit_tree, &hdist_tree, io);
                if (! deflate_huffman (&hlit_tree, &hdist_tree, io)) return 0;
                break;
      case 3  :
                // Reserved
      default :
                return 0;
                break;
    }
  } while (!bfinal);

  // Everything went ok
  return 1;
}


/**
 *
 */
int gzip_decode (io_t *io) {
  int i;


  // Init streams to start position
  io->in_ptr = io->out_length = 0;


  // Read signature and flags
  unsigned char id1 = read_input (io, 8);
  unsigned char id2 = read_input (io, 8);
  unsigned char cm = read_input (io, 8);
  unsigned char fl = read_input (io, 8);

  // Check header signature and compression method
  if (id1 != 0x1F && id2 != 0x8B && cm != 8) return 0;

  read_input (io, 32);  // Skip Mtime
  read_input (io, 8);   // Skip extra flags
  read_input (io, 8);   // Skip operating system type

  // Skip optional extra field
  if ((fl & FLG_FEXTRA) == FLG_FEXTRA) {
    unsigned short extra_length = read_input (io, 16);
    for (i=0; i!=extra_length; i++) read_input (io, 8);
  }

  // Skip optional name field
  if ((fl & FLG_FNAME) == FLG_FNAME) while (read_input (io, 8) != 0) ;

  // Skip optional comment field
  if ((fl & FLG_FCOMMENT) == FLG_FCOMMENT) while (read_input (io, 8) != 0) ;

  // Skip optional header CRC field
  if ((fl & FLG_FHCRC) == FLG_FHCRC) io->in_ptr += (2 * 8);    // Skip 2 crc bytes


  /*
   * We are now at the start of the compressed data
   */

  // Allocate 1K buffer and deflate. Buffer will be resized when needed
  io->out_size = 1024;
  io->out = (unsigned char *)malloc (io->out_size);
  if (! deflate (io)) return 0;

  // Resize buffer to correct length back (do we really need this?)
  io->out = realloc (io->out, io->out_length);

  // OK
  return 1;
}


/**
 *
 */
int main (void) {
  // By using an IO structure, we don't have to walk around with 4 params all the time
  io_t io;

  // Open file
  FILE *f = fopen ("bochsout.txt.gz", "rb");
  if (!f) return 0;

  // Find file size
  fseek (f, 0, SEEK_END);
  io.in_length = ftell (f);
  fseek (f, 0, SEEK_SET);

  // Alloc space
  io.in = (char *)malloc (io.in_length);
  if (! io.in) return 0;

  // Read file
  int ret = fread (io.in, 1, io.in_length, f);
  if (ret != io.in_length) return 0;

  fclose (f);


  // Decode buffer (output is automatically filled
  if (! gzip_decode (&io)) return 0;

  f = fopen ("result", "wb");
  if (!f) return 0;
  if (fwrite (io.out, 1, io.out_length, f) != io.out_length) return 0;
  fclose (f);

  return 0;
}