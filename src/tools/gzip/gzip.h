#ifndef __GZIP_H__
#define __GZIP_H__

typedef struct {
  char         maxbits;
  int          length;
  char         size_table[288];
  unsigned int code_table[288];
} huffman_t;



#define FLG_FTEXT    1 << 0
#define FLG_FHCRC    1 << 1
#define FLG_FEXTRA   1 << 2
#define FLG_FNAME    1 << 3
#define FLG_FCOMMENT 1 << 4

#pragma pack(1)
typedef struct {
  unsigned char  id1;
  unsigned char  id2;
  unsigned char  compression_method;
  unsigned char  flags;
  unsigned long  mtime;
  unsigned char  extra_flags;
  unsigned char  os;
} gzip_header_t;

#pragma pack(1)
typedef struct {
  unsigned char si1;
  unsigned char si2;
  unsigned short length;
  unsigned char *data;
} gzip_subfield_t;

#pragma pack(1)
typedef struct {
  unsigned short length;
  gzip_subfield_t *subfields;
} gzip_header_extra_t;


typedef struct {
  gzip_header_t *header;

  gzip_header_extra_t *extra;   // Header fields or NULL when not present
  unsigned char *filename;      // Filename or NULL when not present
  unsigned char *comment;       // Comment or NULL when not present
  unsigned char *text;          // Text or NULL when not present
  unsigned short crc16;         // crc16 or 0 when not present

  unsigned char *data;          // Data blocks

  unsigned long crc32;          // CRC32 of file
  unsigned long size;           // Size of the original file
} gzip_t;


#define CLEN_MAXBITS 15
#define HLIT_MAXBITS 15
#define HDIST_MAXBITS 15

#define CLEN_TSIZE 19
#define HLIT_TSIZE 288
#define HDIST_TSIZE 30




#endif // __GZIP_H__