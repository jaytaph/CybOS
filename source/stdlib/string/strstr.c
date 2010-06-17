#include <string.h>
#include <_size_t.h>

char *strstr (const char *str1, const char *str2) {
  char c;
  size_t len;

  c = *str2++;
  if (!c) return (char *)str1;

  len = strlen (str2);
  do {
    char sc;

    do {
      sc = *str1++;
      if (!sc) return (char *)0;
    } while (sc != c);
  } while (strncmp (str1, str2, len) != 0);

  return (char *)(str1-1);

}
