#include <_size_t.h>

/*********
 *
 */
void *memset (void *dst, int val, size_t count)
{
  char *temp;

  for (temp=(char *)dst; count!=0; count--)
    *temp++=val;
  return dst;
}


/*********
 *
 */
void *memsetl (void *dst, long val, size_t count)
{
  long *temp;

  for (temp=(long *)dst; count!=0; count--)
    *temp++=val;
  return dst;
}


/*********
 *
 */
void *memsetw (void *dst, int val, size_t count)
{
  unsigned short *temp = (unsigned short *)dst;

  for (; count!=0; count--)
    *temp++=val;
  return dst;
}
