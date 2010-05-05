#ifndef __STDIO_H__
#define __STDIO_H__

  #include <stdarg.h>
  #include <_null.h>

  #define _IONBF       0       // Unbuffered stream
  #define _IOFBF       1       // Fully buffered stream
  #define _IOLBF       2       // Line and buffered stream

  #define BUFSIZ       53

  #define stdin  (_std_streams +0)
  #define stdout (_std_streams +1)
  #define stderr (_std_streams +2)

  #define EOF   (-1)

  #define putchar(CHAR) fputc (CHAR, stdout)

  typedef struct {
    char *buf_base, *buf_ptr;
    unsigned long size, room, flags;
    int handle;
  } FILE;

  extern FILE _std_streams[];

  int vsprintf (char *buffer, const char *fmt, va_list args);
  int sprintf (char *buffer, const char *fmt, ...);
  int vprintf (const char *fmt, va_list args);
  int printf (const char *fmt, ...);
  int fputc (int c, FILE *stream);
  int fflush (FILE *stream);

#endif // __STDIO_H__
