/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * Project: Core CLI + Library-Ready Refactor
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * 
 * PURPOSE
 *   This file is part of a refactor that breaks the monolithic main.c into
 *   small, loosely-coupled modules. The goal is to keep the CLI working today
 *   while preparing for a reusable library and a GTK4 IDE (Umicom Studio).
 *
 * LICENSE
 *   SPDX-License-Identifier: MIT
 *---------------------------------------------------------------------------*/
#ifndef UENG_COMMON_H
#define UENG_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
  static inline int ueng_make_dir(const char* p) { return _mkdir(p); }
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #define PATH_SEP '/'
  #define ACCESS_EXISTS 0
  static inline int ueng_make_dir(const char* p) { return mkdir(p, 0755); }
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------- Small utils ----------------------------------*/
typedef struct {
  char **items;
  size_t count;
  size_t cap;
} StrList;

void   sl_init(StrList*);
void   sl_free(StrList*);
int    sl_push(StrList*, const char* s);

/* FS utils */
int    helper_exists_file(const char* path);
FILE*  ueng_fopen(const char* path, const char* mode);
int    mkpath(const char* path);
int    write_file(const char* path, const char* content);
int    write_text_file_if_absent(const char* path, const char* content);
int    append_file(FILE* dst, const char* src_path);
int    copy_file_binary(const char* src, const char* dst);
int    clean_dir(const char* dir);

/* Strings */
char*  ltrim(char* s);
void   rtrim(char* s);
void   unquote(char* s);
int    ci_cmp(const char* a, const char* b);
int    natural_ci_cmp(const char* a, const char* b);
int    endswith_ic(const char* s, const char* suffix);
void   rel_normalize(char* s);
void   rel_to_native_sep(char* s);

/* Time / naming */
void   build_date_utc(char* out, size_t outsz);
void   build_timestamp_utc(char* out, size_t outsz);
void   slugify(const char* title, char* out, size_t outsz);

/* Minimal YAML helpers */
int    tiny_yaml_get(const char* filename, const char* key, char* out, size_t outsz);
int    tiny_yaml_get_bool(const char* filename, const char* key, int* out);

#ifdef __cplusplus
}
#endif

#endif /* UENG_COMMON_H */
