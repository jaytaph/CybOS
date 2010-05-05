#include <_size_t.h>

char *strncpy (char *dst, const char *src, size_t count)
{
  char *ret_val = dst;

  for (; (*src != '\0') && (count != 0); count--) *dst++ = *src++;
  *dst = '\0';
  return ret_val;
}
