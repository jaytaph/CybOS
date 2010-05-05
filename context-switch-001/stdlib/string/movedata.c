#include <_size_t.h>


void movedata(unsigned short src_sel, unsigned int src_off,
              unsigned short dst_sel, unsigned int dst_off, size_t count)
{
	__asm__ __volatile__(
          "    pushl %%ds\n"
          "    pushl %%es\n"
          "    movw %0,%%es\n"
          "    movw %1,%%ds\n"
/* xxx - handle overlapping src and dst */
          "    cld\n"
/* take care of odd-length (byte) */
          "    shr %%ecx\n"
          "    jnc 1f\n"
          "    movsb\n"
/* take care of word */
          "1:  shr %%ecx\n"
          "    jnc 2f\n"
          "    movsw\n"
/* now copy 32 bits at a time */
          "2:  rep\n"
          "    movsl\n"
          "    popl %%es\n"
          "    popl %%ds\n"
          :
          : "r"(dst_sel), "r"(src_sel), "c"(count),
            "D"(dst_off), "S"(src_off)
        );
}


