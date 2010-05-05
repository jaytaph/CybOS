#include <stdio.h>
#include <_sys.h>

int fflush (FILE *stream)
{
  long bytes_to_write;
  int ret_val;

  ret_val = 0;
  if (stream->size!=0)
  {
    bytes_to_write = stream->size - stream->room;
    if (bytes_to_write != 0)
    {
      if (os_write (stream->handle, stream->buf_base, bytes_to_write)!=bytes_to_write) ret_val=EOF;
      stream->buf_ptr = stream->buf_base;
      stream->room=stream->size;
    }
  }
  return ret_val;
}
