#include <_sys.h>

int os_write (int handle, void *buf, unsigned len)
{
  int ret_val;

  __asm__ __volatile__ (
    "int %1\n\t"
    : "=a"(ret_val)
    : "i"(SYSCALL_INT), "a"(SYS_WRITE), "b"(buf), "c"(len)
  );
  return ret_val;                        
}
