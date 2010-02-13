#include <_size_t.h>

int strncmp (const char *str1, const char *str2, size_t count)
{
  for (; (*str2 != '\0') && (*str1 == *str2) && (count != 0); count--)
  {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}
