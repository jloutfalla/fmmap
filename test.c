#define FMMAP_IMPLEMENTATION
#include "fmmap.h"

#include <stdio.h> /* fmmap.h needs to be included before stdio.h */

int
main(int argc, char *argv[])
{
  const char *str;
  fmmap_file *f = fmmap_open_file("./test.c");
  fmmap_mmap_file((char **)&str, f);

  printf("%s\n", str);

  fmmap_unmap_file((char **)&str, f);
  fmmap_close_file(f);
  
  return 0;
}
