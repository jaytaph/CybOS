#include <stdio.h>
#include <stdlib.h>

/**
 *
 */
int main (void) {
  int i;

  printf ("Hello world from newlib application!");

  i = rand () % 100;
  printf ("A random number under 100 would be: %d\n", i);
  return 5;
}



