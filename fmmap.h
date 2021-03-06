/* fmmap -- Header only cross-platform library to map file into memory

   Copyright (C) 2021 Jean-Baptiste Loutfalla <jb.loutfalla@orange.fr>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>. */

#ifndef FMMAP_H
#define FMMAP_H

#if defined (__unix__) || defined (__unix) || (defined (__APPLE__) && defined (__MACH__))
#define __FMMAP_UNIX__
#endif

#if defined (__WIN32) || defined (__WIN64)
#define __FMMAP_WIN__
#endif

#if defined __FMMAP_UNIX__
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif /* _POSIX_C_SOURCE */
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined __FMMAP_WIN__
#include <Windows.h>
#include <io.h>
#define fstream _fstream
#define fileno _fileno
#endif /* __FMMAP_WIN__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef FMMAPDEF
  #ifdef FMMAP_STATIC
    #define FMMAPDEF static
  #else
    #define FMMAPDEF extern
  #endif
#endif

#ifndef FMMAP_LOG
  #define FMMAP_LOG(...)                          \
    {                                             \
      fprintf(stderr, __VA_ARGS__);               \
      fputc('\n', stderr);                        \
    }
#endif
  
  typedef struct os_spec fmmap_os_spec;
  typedef struct {
    FILE         *stream;
    const char   *name;
    size_t        size;
    fmmap_os_spec *os_spec;
  } fmmap_file;

  FMMAPDEF fmmap_file *fmmap_open_file(const char *restrict file_path,
                                       const char *restrict mode);
  FMMAPDEF int        fmmap_close_file(fmmap_file *file);
  FMMAPDEF int        fmmap_mmap_file(char **restrict buff,
                                      const fmmap_file *restrict file);
  FMMAPDEF int        fmmap_unmap_file(char **restrict buff,
                                       const fmmap_file *restrict file);

#ifdef __cplusplus
}
#endif /* __cplusplus */
  
#endif /* FMMAP_H */

#ifdef FMMAP_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  
  struct os_spec {
    int fd;
#ifdef __FMMAP_WIN__
    HANDLE handle_file;
    HANDLE handle_map;
#endif /* __FMMAP_WIN__ */
  };

  static int
  fmmap_file_release_os(fmmap_file *file)
  {
    int ret;
    fmmap_os_spec *spec;

    if (file == NULL)
      {
        FMMAP_LOG("No file structure provided");
        return -1;
      }

    if (file->os_spec == NULL)
      {
        FMMAP_LOG("`%s`: No OS specific structure allocated", file->name);
        return -1;
      }

    spec = file->os_spec;
    ret = 0;
  
#if defined (__FMMAP_WIN__)
    ret = 0;
    if (spec->handle_map != INVALID_HANDLE_VALUE)
      {
        ret = CloseHandle(spec->handle_map);
        ret = (ret == 0);
      }

    spec->handle_file = INVALID_HANDLE_VALUE;
#endif /* __FMMAP_WIN__ */
  
    free(spec);
    file->os_spec = NULL;
  
    return ret;
  }
  
  static int
  fmmap_init_file_os(fmmap_file *file)
  {
    int ret;
    fmmap_os_spec *spec;

    if (file == NULL)
      {
        FMMAP_LOG("No file structure provided");
        return -1;
      }
  
    if (file->os_spec != NULL)
      {
        FMMAP_LOG("`%s`: File OS specific structure already allocated", file->name);
        fmmap_file_release_os(file);
      }
  
    spec = (fmmap_os_spec *)malloc(sizeof(fmmap_os_spec));
    if (spec == NULL)
      {
        FMMAP_LOG("`%s`: Can't allocate OS specific structure", file->name);
        return -1;
      }
      
    file->os_spec = spec;
  
    spec->fd = fileno(file->stream);
    ret = (spec->fd == -1);
    if (ret != 0)
      FMMAP_LOG("`%s`: Can't retrieve the file descriptor", file->name);
  
#if defined(__FMMAP_WIN__)
    spec->handle_file = (HANDLE)_get_osfhandle(spec->fd);
    ret = (spec->handle_file == INVALID_HANDLE_VALUE);
    if (ret != 0)
      FMMAP_LOG("`%s`: Invalid file handler retrieved", file->name);

    spec->handle_map = INVALID_HANDLE_VALUE;
#endif /* __FMMAP_WIN__ */

    return ret;
  }
  
  static int
  fmmap_file_size(fmmap_file *file)
  {
    int ret = 0;
    fmmap_os_spec *spec;
  
    if (file == NULL)
      {
        FMMAP_LOG("No file structure provided");
        return -1;
      }

    spec = (fmmap_os_spec *)file->os_spec;
    if (spec == NULL)
      {
        FMMAP_LOG("`%s`: No OS specific structure allocated", file->name);
        return -1;
      }
  
#if defined(__FMMAP_UNIX__)
    struct stat stats;
  
    ret = fstat(spec->fd, &stats);
    if (ret != 0)
      FMMAP_LOG("`%s`: Can't retrieve file stats", file->name);

    if (ret == 0)
      file->size = stats.st_size;
#elif defined(__FMMAP_WIN__)
    ret = _filelength(spec->fd);

    if (ret != -1)
      {
        file->size = ret;
        ret = 0;
      }
    else
      {
        FMMAP_LOG("`%s`: Can't retrieve the size", file->name);
      }
#endif /* __FMMAP_WIN__ */
  
    return ret;
  }

  FMMAPDEF fmmap_file *
  fmmap_open_file(const char *restrict file_path, const char *restrict mode)
  {
    size_t length;
    FILE *f;
    fmmap_file *file;

    if (file_path == NULL || mode == NULL)
      {
        FMMAP_LOG("No file path and/or no opening mode provided");
        return NULL;
      }
  
    f = fopen(file_path, mode);
    if (f == NULL)
      {
        FMMAP_LOG("`%s`: Can't open the file", file_path);
        return NULL;
      }

    file = (fmmap_file *)malloc(sizeof(fmmap_file));
    length = strlen(file_path) + 1; 

    file->stream = f;
    file->name = malloc(length);
    strncpy((void *)file->name, file_path, length);

    file->os_spec = NULL;
    fmmap_init_file_os(file);

    file->size = 0;
    fmmap_file_size(file);

    return file;
  }
  
  FMMAPDEF int
  fmmap_close_file(fmmap_file *file)
  {
    int ret;

    if (file == NULL)
      {
        FMMAP_LOG("No file structure provided");
        return -1;
      }
  
    ret = fclose(file->stream);
    if (ret != 0)
      {
        FMMAP_LOG("`%s`: Can't close the file", file->name);
        return ret;
      }

    free((void*)file->name);
  
    file->size = 0;
  
    ret = fmmap_file_release_os(file);
  
    free(file);
  
    return ret;
  }
  
  FMMAPDEF int
  fmmap_mmap_file(char **restrict buff, const fmmap_file *restrict file)
  {
    char *tmp_buff = NULL;
    void *test_ptr = NULL;
    fmmap_os_spec *spec;

    if (file == NULL || buff == NULL)
      {
        FMMAP_LOG("No file path and/or no opening mode provided");
        return -1;
      }

    spec = (fmmap_os_spec *)file->os_spec;
    if (spec == NULL)
      {
        FMMAP_LOG("`%s`: No OS specific structure allocated", file->name);
        return -1;
      }
  
#if defined(__FMMAP_UNIX__)
    tmp_buff = mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, spec->fd, 0);
    test_ptr = MAP_FAILED;
#elif defined(__FMMAP_WIN__)
    spec->handle_map = CreateFileMapping(spec->handle_file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (spec->handle_map == INVALID_HANDLE_VALUE)
      {
        FMMAP_LOG("`%s`: Invalid handler retrieved when creating memory mapping", file->name);
        return -1;
      }
  
    tmp_buff = MapViewOfFile(spec->handle_map, FILE_MAP_READ, 0, 0, 0);
    test_ptr = NULL;
#endif
  
    if (tmp_buff == test_ptr)
      {
        FMMAP_LOG("`%s`: Can't memory map", file->name);
        return -1;
      }

    *buff = tmp_buff;
  
    return 0;
  }
  
  FMMAPDEF int
  fmmap_unmap_file(char **restrict buff, const fmmap_file *restrict file)
  {
    int ret = -1;
    fmmap_os_spec *spec;

    if (file == NULL || buff == NULL || *buff == NULL)
      {
        FMMAP_LOG("No file structure and/or buffer provided");
        return ret;
      }
  
    if (file->size == 0)
      {
        FMMAP_LOG("`%s`: The size of the file is invalid", file->name);
        return ret;
      }

    spec = (fmmap_os_spec *)file->os_spec;
    if (spec == NULL)
      {
        FMMAP_LOG("`%s`: No OS specific structure allocated", file->name);
        return ret;
      }
  
#if defined(__FMMAP_UNIX__)
    ret = munmap(*buff, file->size);
#elif defined(__FMMAP_WIN__)
    ret = CloseHandle(spec->handle_map);
    if (ret != 0)
      {
        spec->handle_map = INVALID_HANDLE_VALUE;
        ret = 0;
      }
#endif /* __FMMAP_WIN__ */
  
    if (ret != 0)
      {
        FMMAP_LOG("`%s`: Can't unmap memory", file->name);
        return ret;
      }
  
    *buff = NULL;
  
    return ret;
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FMMAP_IMPLEMENTATION */
