/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: src/common.c
 * Purpose: Cross-platform helpers & small utilities
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * License: MIT
 *---------------------------------------------------------------------------*/
#include "ueng/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <shellapi.h>
  #include <io.h>
  #include <direct.h>
  #ifndef strdup
    #define strdup _strdup
  #endif
  #define ACCESS(p,m) _access(p,m)
#else
  #include <unistd.h>
  #include <dirent.h>
  #define ACCESS(p,m) access(p,m)
#endif

/*------------------------------ I/O wrappers --------------------------------*/
/* Thin wrappers so the rest of the app has fewer #ifdefs. */

FILE* ueng_fopen(const char* path, const char* mode){
  return fopen(path, mode);
}

int file_exists(const char* path){
  return (ACCESS(path, ACCESS_EXISTS) == 0) ? 1 : 0;
}

int dir_exists(const char* path){
  struct stat st;
  if (stat(path, &st) != 0) return 0;
#ifdef _WIN32
  return (st.st_mode & _S_IFDIR) ? 1 : 0;
#else
  return S_ISDIR(st.st_mode) ? 1 : 0;
#endif
}

/* mkpath / mkpath_parent create folders like `mkdir -p`. Safe to call repeatedly. */
static int mkpath_single(const char* path){
  if (dir_exists(path)) return 0;
  return make_dir(path) == 0 ? 0 : -1;
}

int mkpath(const char* path){
  if (!path || !*path) return 0;
  char tmp[PATH_MAX];
  size_t len = strlen(path);
  if (len >= sizeof(tmp)) return -1;
  memcpy(tmp, path, len+1);

  /* Walk and create intermediate components. We treat both slashes on Windows. */
  for (size_t i=1; i<len; ++i){
#ifdef _WIN32
    if (tmp[i]=='/' || tmp[i]=='\\'){
#else
    if (tmp[i]=='/'){
#endif
      char c = tmp[i];
      tmp[i] = '\0';
      if (mkpath_single(tmp) != 0) return -1;
      tmp[i] = c;
    }
  }
  return mkpath_single(tmp);
}

int mkpath_parent(const char* filepath){
  if (!filepath || !*filepath) return 0;
  char buf[PATH_MAX];
  size_t n = strlen(filepath);
  if (n >= sizeof(buf)) return -1;
  memcpy(buf, filepath, n+1);

  /* Chop the last path component and create the dir chain. */
  char* sep = strrchr(buf, PATH_SEP);
#ifdef _WIN32
  char* alt = strrchr(buf, '/');
  if (alt && (!sep || alt > sep)) sep = alt;
#endif
  if (!sep) return 0;
  *sep = '\0';
  return mkpath(buf);
}

int write_text_file(const char* path, const char* text){
  if (mkpath_parent(path) != 0) return -1;
  FILE* f = ueng_fopen(path, "wb");
  if (!f) return -1;
  /* Write exactly what we're given; the md/html writers own formatting. */
  fputs(text ? text : "", f);
  fclose(f);
  return 0;
}

int write_text_file_if_absent(const char* path, const char* text){
  if (file_exists(path)) return 0;
  return write_text_file(path, text);
}

/* Optional convenience used by some call sites */
int write_file(const char* path, const char* content){
  return write_text_file(path, content);
}

/*------------------------------ small utils --------------------------------*/
/* UTC-only date helpers keep builds reproducible across machines/timezones. */

void build_date_utc(char* out, size_t outsz){
  time_t now=time(NULL);
  struct tm* tm=gmtime(&now);
  strftime(out, outsz, "%Y-%m-%d", tm);
}

void build_timestamp_utc(char* out, size_t outsz){
  time_t now=time(NULL);
  struct tm* tm=gmtime(&now);
  strftime(out, outsz, "%Y-%m-%dT%H-%M-%SZ", tm);
}

/* Improved slugify:
   - Lowercases ASCII letters
   - Keeps [a-z0-9 _ - +]
   - Turns everything else into single dashes
   - Trims leading/trailing dashes */
void slugify(const char* in, char* out, size_t outsz){
  size_t j=0; int prev_dash = 0;
  if (!in || !out || outsz==0){ if (outsz) out[0]='\0'; return; }

  for (size_t i=0; in[i] && j+1<outsz; ++i){
    unsigned char c=(unsigned char)in[i];
    if (isalnum(c) || c=='+' || c=='-' || c=='_'){
      if (c>='A' && c<='Z') c = (unsigned char)(c - 'A' + 'a');
      out[j++] = (char)c;
      prev_dash = 0;
    }else{
      /* Collapse any run of non-allowed chars into a single dash */
      if (!prev_dash){
        out[j++] = '-';
        prev_dash = 1;
      }
    }
  }
  if (j>0 && out[j-1]=='-') j--;  /* strip trailing '-' */
  out[j] = '\0';

  /* Strip any leading dashes */
  if (out[0]=='-'){
    size_t k = 0; while (out[k]=='-') k++;
    size_t len = strlen(out + k);
    memmove(out, out + k, len + 1);
  }
}

/* Tiny string helpers used by the YAML-ish parsing. */
char* ltrim(char* s){
  if (!s) return s;
  while (*s && isspace((unsigned char)*s)) ++s;
  return s;
}
void rtrim(char* s){
  if (!s) return;
  size_t n = strlen(s);
  while (n && isspace((unsigned char)s[n-1])) s[--n]='\0';
}
void unquote(char* s){
  if (!s) return;
  size_t n = strlen(s);
  if (n>=2 && ((s[0]=='"' && s[n-1]=='"') || (s[0]=='\'' && s[n-1]=='\''))){
    memmove(s, s+1, n-2);
    s[n-2]='\0';
  }
}

/* In-place "replace all" helper; safe within a fixed-size buffer. */
void str_replace_inplace(char* buf, size_t bufsz, const char* pat, const char* rep){
  if (!buf || !pat || !rep) return;
  char tmp[65536];
  size_t blen = strlen(buf), plen=strlen(pat), rlen=strlen(rep);
  if (blen >= sizeof(tmp)-1) blen = sizeof(tmp)-1;

  const char* s = buf;
  char* d = tmp;
  while (*s && (size_t)(d-tmp)+1 < sizeof(tmp)){
    if (plen && strncmp(s, pat, plen)==0){
      if ((size_t)(d-tmp) + rlen >= sizeof(tmp)) break;
      memcpy(d, rep, rlen); d += rlen; s += plen;
    }else{
      *d++ = *s++;
    }
  }
  *d = '\0';
  strncpy(buf, tmp, bufsz-1); buf[bufsz-1]='\0';
}

/*------------------------- natural sort comparator -------------------------*/
/* qsort comparator that sorts strings like "ch2" before "ch10" (case-insens.). */
static int nat_value(const unsigned char** p){
  int v = 0; int any=0;
  while (isdigit(**p)){ v = v*10 + (**p - '0'); (*p)++; any=1; }
  return any ? v : -1;
}

int qsort_nat_ci_cmp(const void* A, const void* B){
  const char* a = *(const char* const*)A;
  const char* b = *(const char* const*)B;
  const unsigned char* pa = (const unsigned char*)a;
  const unsigned char* pb = (const unsigned char*)b;

  for(;;){
    if (*pa=='\0' && *pb=='\0') return 0;
    if (*pa=='\0') return -1;
    if (*pb=='\0') return  1;

    if (isdigit(*pa) && isdigit(*pb)){
      const unsigned char* sa=pa; const unsigned char* sb=pb;
      int va = nat_value(&pa);
      int vb = nat_value(&pb);
      if (va != vb) return (va < vb) ? -1 : 1;
      int la = (int)(pa - sa);
      int lb = (int)(pb - sb);
      if (la != lb) return (la < lb) ? -1 : 1;
      continue;
    }

    unsigned char ca = (unsigned char)tolower(*pa++);
    unsigned char cb = (unsigned char)tolower(*pb++);
    if (ca != cb) return (ca < cb) ? -1 : 1;
  }
}

/*------------------------------- StrList -----------------------------------*/
void sl_init(StrList* sl){
  if (!sl) return;
  sl->items=NULL; sl->count=0; sl->cap=0;
}
void sl_free(StrList* sl){
  if (!sl) return;
  for (size_t i=0;i<sl->count;++i) free(sl->items[i]);
  free(sl->items);
  sl->items=NULL; sl->count=sl->cap=0;
}
int sl_push(StrList* sl, const char* s){
  if (!sl || !s) return -1;
  if (sl->count == sl->cap){
    size_t ncap = sl->cap ? sl->cap*2 : 16;
    char** nitems = (char**)realloc(sl->items, ncap*sizeof(char*));
    if(!nitems) return -1;
    sl->items = nitems; sl->cap = ncap;
  }
  sl->items[sl->count] = strdup(s);
  if (!sl->items[sl->count]) return -1;
  sl->count++;
  return 0;
}

/*----------------------------- shell helpers -------------------------------*/
/* NOTE: On Windows we treat any nonzero exit as failure. On POSIX we also
   inspect WEXITSTATUS to propagate the real exit code. */
int exec_cmd(const char* cmdline){
  if (!cmdline || !*cmdline) return -1;
  int rc = system(cmdline);
#ifdef _WIN32
  if (rc == -1) return -1;
  return (rc == 0) ? 0 : 1;
#else
  if (rc == -1) return -1;
  return (WIFEXITED(rc) && WEXITSTATUS(rc)==0) ? 0 : 1;
#endif
}

/* Convert relative to absolute path on both platforms. */
int path_abs(const char* in, char* out, size_t outsz){
  if (!in || !*in || !out || outsz==0){ if(outsz) out[0]='\0'; return -1; }
#ifdef _WIN32
  DWORD n = GetFullPathNameA(in, (DWORD)outsz, out, NULL);
  if (!n || n >= outsz){ if(outsz) out[0]='\0'; return -1; }
  return 0;
#else
  char* p = realpath(in, out);
  if (!p){ if(outsz) out[0]='\0'; return -1; }
  return 0;
#endif
}

/* Turn absolute file path into a file:// URL that browsers can open. */
void path_to_file_url(const char* abs, char* out, size_t outsz){
  if (!abs || !out || outsz==0){ if(outsz) out[0]='\0'; return; }
#ifdef _WIN32
  char tmp[PATH_MAX]; strncpy(tmp, abs, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
  for (char* p=tmp; *p; ++p) if (*p=='\\') *p='/';
  snprintf(out, outsz, "file:///%s", tmp);
#else
  snprintf(out, outsz, "file://%s", abs);
#endif
}

/* Open local file or URL in the default system browser. */
int open_in_browser(const char* path_or_url){
#ifdef _WIN32
  wchar_t wbuf[1024];
  MultiByteToWideChar(CP_UTF8, 0, path_or_url, -1, wbuf, 1024);
  INT_PTR r = (INT_PTR)ShellExecuteW(NULL, L"open", wbuf, NULL, NULL, SW_SHOWNORMAL);
  return (r > 32) ? 0 : 1;
#else
  char cmd[2048];
  snprintf(cmd,sizeof(cmd),
           "xdg-open '%s' >/dev/null 2>&1 || open '%s' >/dev/null 2>&1",
           path_or_url, path_or_url);
  return exec_cmd(cmd);
#endif
}

#ifdef _WIN32
/* Optional console UTF-8 setup for nicer Windows output. */
void ueng_console_utf8(void){
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
}
#endif

/*---------------------------- book.yaml parser -----------------------------*/
/* (Parsing helpers live in main.c; this file hosts cross-platform utilities.) */
/*------------------------------ End of file --------------------------------*/