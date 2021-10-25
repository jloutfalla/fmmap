#include <stdio.h>

#define FMMAP_IMPLEMENTATION
#include "fmmap.h"

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
