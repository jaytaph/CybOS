#include <_size_t.h>

int strcmp (const char *str1, const char *str2)
{
  while ((*str2 != '\0') && (*str1 == *str2))
  {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}
