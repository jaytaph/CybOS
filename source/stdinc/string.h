#ifndef __STRING_H__
#define __STRING_H__

#include <_size_t.h>

  int strcmp (const char *str1, const char *str2);
  int strncmp (const char *str1, const char *str2, size_t count);

  char *strstr (const char *str1, const char *str2);

  char *strcpy (char *dst, const char *src);
  char *strncpy (char *dst, const char *src, size_t count);
  size_t strlen (const char *str);

  void *memcpy (void *dst_ptr, const void *src_ptr, size_t count);

  void *memset  (void *dst, int val, size_t count);
  void *memsetw (void *dst, int val, size_t count);
  void *memsetl (void *dst, long val, size_t count);

  void movedata (unsigned short src_seg, unsigned int src_off,
                 unsigned short dst_seg, unsigned int dst_off, size_t count);

#endif // __STRING_H__
