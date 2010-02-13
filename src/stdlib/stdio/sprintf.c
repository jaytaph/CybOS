#include <stdio.h>

/****
 *
 */
int sprintf (char *buffer, const char *fmt, ...)
{
  va_list args;
  int ret_val;

  va_start (args, fmt);
  ret_val = vsprintf (buffer, fmt, args);
  va_end (args);
  return ret_val;
}

