#ifndef UENG_COMMON_H
#define UENG_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

/* --- Platform + path bits ---------------------------------------------- */
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <direct.h>
  #include <io.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  #define PATH_SEP '\\'
  #define access _access
  #define ACCESS_EXISTS 0
  typedef SOCKET sock_t;
  #define SOCK_INVALID INVALID_SOCKET
  #define closesock closesocket
  static inline int ueng_make_dir(const char* p){ return _mkdir(p); }
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #define PATH_SEP '/'
  #define ACCESS_EXISTS 0
  typedef int sock_t;
  #define SOCK_INVALID (-1)
  #define closesock close
  static inline int ueng_make_dir(const char* p){ return mkdir(p, 0755); }
#endif

/* --- File existence + fopen -------------------------------------------- */
int  ueng_file_exists(const char* path);
FILE* ueng_fopen(const char* path, const char* mode);

/* inet pton v4 wrapper (returns 1 on success) */
int  ueng_inet_pton4(const char* src, void* dst);

/* --- Strings ------------------------------------------------------------ */
char* ueng_ltrim(char* s);
void  ueng_rtrim(char* s);
void  ueng_unquote(char* s);

/* slug + date stamps */
void  ueng_slugify(const char* title, char* out, size_t outsz);
void  ueng_build_date_utc(char* out, size_t outsz);
void  ueng_build_timestamp_utc(char* out, size_t outsz);

/* mkpath + clean_dir */
int   ueng_mkpath(const char* path);
int   ueng_clean_dir(const char* dir);

/* path separator normalization for relative paths */
void  ueng_rel_normalize(char* s);
void  ueng_rel_to_native_sep(char* s);

/* endswith (case-insensitive) */
int   ueng_endswith_ic(const char* s, const char* suffix);

/* --- Simple string list ------------------------------------------------- */
typedef struct {
  char **items;
  size_t count;
  size_t cap;
} UengStrList;

void  ueng_sl_init(UengStrList* sl);
void  ueng_sl_free(UengStrList* sl);
int   ueng_sl_push(UengStrList* sl, const char* s);

/* natural, case-insensitive compare helpers for qsort */
int   ueng_qsort_nat_ci_cmp(const void* A, const void* B);

#endif /* UENG_COMMON_H */

