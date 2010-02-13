#include <_size_t.h>

size_t strlen (const char *str)
{
  size_t ret_val;

  for (ret_val=0; *str!='\0'; str++) ret_val++;

  return ret_val;
}
