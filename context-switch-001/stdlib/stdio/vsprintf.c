#include <_printf.h>

/****
 *
 */
int vsprintf_help (char c, void **ptr)
{
  char *dst;

  dst=*ptr;
  *dst++=c;
  *ptr=dst;
  return 0;
}

/****
 *
 */
int vsprintf (char *buffer, const char *fmt, va_list args)
{
  return do_printf (fmt, args, vsprintf_help, (void *)&buffer);
}
