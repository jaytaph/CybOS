#include <stdio.h>

int printf (const char *fmt, ...)
{
  va_list args;
  int ret_val;

  va_start (args, fmt);
  ret_val = vprintf (fmt, args);
  va_end (args);

  return ret_val;
}
