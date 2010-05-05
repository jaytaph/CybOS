#ifndef ___PRINTF_H__
#define ___PRINTF_H__

  #include <stdarg.h>

  typedef int (*fnptr)(char c, void **helper);

  int do_printf (const char *fmt, va_list args, fnptr fn, void *ptr);

#endif // ___PRINTF_H__
