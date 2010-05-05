#include <stdio.h>

char _stdout_buffer[BUFSIZ];

FILE _std_streams[] = {
  { NULL,           NULL,           0,      0,      0,      0 },
  { _stdout_buffer, _stdout_buffer, BUFSIZ, BUFSIZ, _IOLBF, 1 },
  { NULL,           NULL,           0,      0,      0,      2 }
};
