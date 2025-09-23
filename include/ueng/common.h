/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: include/ueng/common.h
 * Purpose: Platform shims, portable filesystem & string utilities, process exec.
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * License: MIT
 *---------------------------------------------------------------------------*/
#ifndef UENG_COMMON_H
#define UENG_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <direct.h>
  #include <io.h>
  #define PATH_SEP '\\'
  #define access _access
  #define ACCESS_EXISTS 0
  static inline int make_dir(const char* p){ return _mkdir(p); }
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
  #define PATH_SEP '/'
  #define ACCESS_EXISTS 0
  static inline int make_dir(const char* p){ return mkdir(p, 0755); }
#endif

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------ String list ---------------------------------*/
typedef struct {
  char **items;
  size_t count;
  size_t cap;
} StrList;

void   sl_init(StrList* sl);
void   sl_free(StrList* sl);
int    sl_push(StrList* sl, const char* s);
int    qsort_nat_ci_cmp(const void* A, const void* B);

/*------------------------------ Filesystem ----------------------------------*/
int    file_exists(const char* path);
FILE*  ueng_fopen(const char* path, const char* mode);
int    write_text_file_if_absent(const char* path, const char* content);
int    write_file(const char* path, const char* content);
int    copy_file_binary(const char* src, const char* dst);
int    write_gitkeep(const char* dir);
int    mkpath(const char* path);      /* mkdir -p */
int    clean_dir(const char* dir);    /* rm -rf children */

/*------------------------------ Time/format --------------------------------*/
void   build_date_utc(char* out, size_t outsz);        /* YYYY-MM-DD */
void   build_timestamp_utc(char* out, size_t outsz);   /* YYYY-MM-DDThh-mm-ssZ */
void   slugify(const char* title, char* out, size_t outsz);

/*------------------------------ Strings ------------------------------------*/
char*  ltrim(char* s);
void   rtrim(char* s);
void   unquote(char* s);

/*------------------------------ Exec/helpers -------------------------------*/
int    exec_cmd(const char* cmdline);
int    path_abs(const char* in, char* out, size_t outsz);
void   path_to_file_url(const char* abs, char* out, size_t outsz);

#ifdef _WIN32
void   ueng_console_utf8(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* UENG_COMMON_H */
