#include <stdio.h>
#include <_printf.h>

/****
 *
 */
int vprintf_help (char c, void **ptr)
{
  putchar (c);
  return 0;
}

/****
 *
 */
int vprintf (const char *fmt, va_list args)
{
  return do_printf (fmt, args, vprintf_help, NULL);
}
