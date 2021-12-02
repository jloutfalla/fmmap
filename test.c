#define FMMAP_IMPLEMENTATION
#include "fmmap.h"

#include <stdio.h> /* fmmap.h needs to be included before stdio.h */
#include <stdlib.h>

static int
test(const char *path)
{
  const char *str;

  fmmap_file *f = fmmap_open_file(path, "r");
  if (f == NULL)
    return 1;
  
  if (fmmap_mmap_file((char **)&str, f) != 0)
    {
      fmmap_close_file(f);
      return 1;
    }

  printf("%s\n", str);

  fmmap_unmap_file((char **)&str, f);
  fmmap_close_file(f);

  return 0;
}

int
main(int argc, char *argv[])
{
  int ret;
  
  {
    ret = test("./test.c");
    printf("Test 1: %d\n\n", ret == 0);
  }
  
  {
    ret = test("./non_existing.c");
    printf("Test 2: %d\n", ret == 0);
  }
  
  return 0;
}
