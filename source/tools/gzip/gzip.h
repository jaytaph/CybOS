#ifndef __GZIP_H__
#define __GZIP_H__

// Huffman tree
typedef struct {
  char         maxbits;       // Maximum number of bits in code_table
  int          length;        // Number of items in *_tables
  char         size_table[288];
  unsigned int code_table[288];
} huffman_t;

typedef struct {
  unsigned int code;
  unsigned int length;
  unsigned int readbits;
} deflate_table_t;

#define FLG_FTEXT    1 << 0
#define FLG_FHCRC    1 << 1
#define FLG_FEXTRA   1 << 2
#define FLG_FNAME    1 << 3
#define FLG_FCOMMENT 1 << 4


#define CLEN_MAXBITS 15
#define HLIT_MAXBITS 15
#define HDIST_MAXBITS 15

#define CLEN_TSIZE 19
#define HLIT_TSIZE 288
#define HDIST_TSIZE 30


// Input/Ouput structure
typedef struct {
  // Preset
  unsigned char *in;         // Must be set prior to calling gzip_deflate
  unsigned int  in_length;   // Length in BYTES of input. Must be set priot to calling gzip_deflate

  // Internal
  unsigned int  in_ptr;      // This pointer is in BITS, not BYTES!
  unsigned int  out_size;    // Holds current size of allocated room

  // Return values
  unsigned char *out;
  unsigned int  out_length;  // Holds the size in BYTES!
} io_t;




#endif // __GZIP_H__