# FMmap - File Memory Mapping Library

`fmmap` is a simple header-only and cross-platform library to map file into memory.

## Quick start

Just copy [fmmap.h](./fmmap.h) into your project and include it like this:

```c 
#define FMMAP_IMPLEMENTATION
#include "fmmap.h"
```

`fmmap` is an [stb-style header-only library](https://github.com/nothings/stb), so 
if you want more information about `FMMAP_IMPLEMENTATION`, please see
[stb_howto.txt](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt).

For an example on how to use this library, check [test.c](./test.c).
