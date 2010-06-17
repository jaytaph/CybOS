#include <_sys.h>

int getch (void)
{
  int ret_val;

  __asm__ __volatile__ (
    "int %1\n"
    : "=a"(ret_val)
    : "i" (SYSCALL_INT), "a" (SYS_GETCH)
  );
  return ret_val;
}
