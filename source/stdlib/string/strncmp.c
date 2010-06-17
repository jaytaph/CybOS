#include <_size_t.h>

int strncmp (const char *str1, const char *str2, size_t count) {
  if (count == 0) return 0;

  do {
      if (*str1 != *str2++) return (*str1-*(str2-1));
      if (*str1++ == 0) break;
  } while (--count != 0);
  return 0;
}
