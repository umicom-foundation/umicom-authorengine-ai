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

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <shellapi.h>
  #include <shlwapi.h>
  #pragma comment(lib, "Shlwapi.lib")
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <dirent.h>
  #include <unistd.h>
#endif

/*------------------------------- String utils -------------------------------*/

/* Replace all occurences of 'from' with 'to' in-place (bounded by buf_sz) */
int str_replace_inplace(char* buf, size_t buf_sz, const char* from, const char* to){
  if (!buf || !from || !to) return -1;
  size_t from_len = strlen(from), to_len = strlen(to);
  if (from_len == 0) return 0;
  char* p = strstr(buf, from);
  while (p){
    /* shift tail if needed */
    size_t tail = strlen(p + from_len);
    if (to_len != from_len){
      if (to_len > from_len){
        size_t need = (size_t)(p - buf) + to_len + tail + 1;
        if (need > buf_sz) return -1;
        memmove(p + to_len, p + from_len, tail + 1);
      } else {
        memmove(p + to_len, p + from_len, tail + 1);
      }
    }
    memcpy(p, to, to_len);
    p = strstr(p + to_len, from);
  }
  return 0;
}

/* case-insensitive natural-order chunk comparator for qsort */
static int natcmp_chunk(const char** a, const char** b){
  const unsigned char *s1=(const unsigned char*)*a, *s2=(const unsigned char*)*b;
  while (*s1 && *s2){
    if (isdigit(*s1) && isdigit(*s2)){
      /* compare number runs as integers */
      unsigned long long n1=0, n2=0;
      while (isdigit(*s1)){ n1 = n1*10 + (*s1 - '0'); s1++; }
      while (isdigit(*s2)){ n2 = n2*10 + (*s2 - '0'); s2++; }
      if (n1 < n2) return -1;
      if (n1 > n2) return  1;
    }else{
      int c1 = tolower(*s1), c2 = tolower(*s2);
      if (c1 < c2) return -1;
      if (c1 > c2) return  1;
      s1++; s2++;
    }
  }
  if (*s1) return 1;
  if (*s2) return -1;
  return 0;
}
int qsort_nat_ci_cmp(const void* a, const void* b){
  const char* sa = *(const char* const*)a;
  const char* sb = *(const char* const*)b;
  const char* pa = sa; const char* pb = sb;
  return natcmp_chunk(&pa, &pb);
}

/* slugify: ascii-ish slug for folder names (very safe) */
void slugify(const char* in, char* out, size_t out_sz){
  if (!in || !out || out_sz==0){ return; }
  size_t j=0;
  int need_dash=0;
  for (const unsigned char* p=(const unsigned char*)in; *p && j+1<out_sz; ++p){
    unsigned char c = *p;
    if (isalnum(c)){
      out[j++] = (char)tolower(c);
      need_dash = 0;
    } else if (c==' ' || c=='_' || c=='-' || c=='.'){
      if (!need_dash && j>0){
        out[j++]='-';
        need_dash=1;
      }
    } else {
      /* drop other punctuation/accents for maximum portability */
      if (!need_dash && j>0){
        out[j++]='-';
        need_dash=1;
      }
    }
  }
  /* trim trailing dash */
  while (j>0 && out[j-1]=='-') j--;
  out[j]='\0';
}

/*----------------------------- Path / Filesystem ----------------------------*/

int file_exists(const char* p){
#ifdef _WIN32
  DWORD a = GetFileAttributesA(p);
  return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st; return (stat(p,&st)==0) && S_ISREG(st.st_mode);
#endif
}

int dir_exists(const char* p){
#ifdef _WIN32
  DWORD a = GetFileAttributesA(p);
  return (a != INVALID_FILE_ATTRIBUTES) && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st; return (stat(p,&st)==0) && S_ISDIR(st.st_mode);
#endif
}

int mkpath(const char* p){
  if (!p || !*p) return -1;
#ifdef _WIN32
  char tmp[PATH_MAX]; strncpy(tmp, p, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
  for (char* q=tmp; *q; ++q){
    if (*q=='/' || *q=='\\') *q='\\';
  }
  for (char* q=tmp+1; *q; ++q){
    if (*q=='\\'){
      char saved = *q; *q='\0';
      if (!dir_exists(tmp)){
        if (make_dir(tmp) != 0 && !dir_exists(tmp)) return -1;
      }
      *q=saved;
    }
  }
  if (!dir_exists(tmp)){
    if (make_dir(tmp) != 0 && !dir_exists(tmp)) return -1;
  }
  return 0;
#else
  char tmp[PATH_MAX]; strncpy(tmp, p, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
  for (char* q=tmp+1; *q; ++q){
    if (*q=='/'){
      *q='\0';
      if (!dir_exists(tmp)){
        if (mkdir(tmp, 0755) != 0 && !dir_exists(tmp)) return -1;
      }
      *q='/';
    }
  }
  if (!dir_exists(tmp)){
    if (mkdir(tmp, 0755) != 0 && !dir_exists(tmp)) return -1;
  }
  return 0;
#endif
}

int mkpath_parent(const char* path){
  if (!path || !*path) return -1;
  char buf[PATH_MAX]; strncpy(buf, path, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
  for (int i=(int)strlen(buf)-1; i>=0; --i){
#ifdef _WIN32
    if (buf[i]=='\\' || buf[i]=='/'){
#else
    if (buf[i]=='/'){
#endif
      buf[i]='\0';
      return mkpath(buf);
    }
  }
  return 0;
}

FILE* ueng_fopen(const char* path, const char* mode){
#ifdef _WIN32
  /* open with UTF-8 path on Windows */
  wchar_t wpath[PATH_MAX], wmode[16];
  MultiByteToWideChar(CP_UTF8,0,path,-1,wpath,PATH_MAX);
  MultiByteToWideChar(CP_UTF8,0,mode,-1,wmode,16);
  FILE* f = _wfopen(wpath, wmode);
  return f;
#else
  return fopen(path, mode);
#endif
}

int write_text_file(const char* path, const char* text){
  if (mkpath_parent(path) != 0) return -1;
  FILE* f = ueng_fopen(path, "wb");
  if (!f) return -1;
  size_t n = fwrite(text, 1, strlen(text), f);
  fclose(f);
  return (n == strlen(text)) ? 0 : -1;
}

int write_text_file_if_absent(const char* path, const char* text){
  if (file_exists(path)) return 0;
  return write_text_file(path, text ? text : "");
}

int read_text_file(const char* path, char* out, size_t outsz){
  FILE* f = ueng_fopen(path, "rb");
  if (!f) return -1;
  size_t n = fread(out, 1, outsz-1, f);
  fclose(f);
  out[n]='\0';
  return 0;
}

/* Open a file or URL in the default system handler (browser, etc.). */
int open_in_browser(const char* path_or_url){
#ifdef _WIN32
  wchar_t wbuf[PATH_MAX];
  MultiByteToWideChar(CP_UTF8,0,path_or_url,-1,wbuf,PATH_MAX);
  HINSTANCE r = ShellExecuteW(NULL, L"open", wbuf, NULL, NULL, SW_SHOWNORMAL);
  return ((INT_PTR)r) > 32 ? 0 : -1;
#else
  char cmd[PATH_MAX + 64];
  snprintf(cmd, sizeof(cmd),
#ifdef __APPLE__
           "open '%s'",
#else
           "xdg-open '%s' >/dev/null 2>&1",
#endif
           path_or_url);
  return system(cmd);
#endif
}

int exec_cmd(const char* cmdline){
#ifdef _WIN32
  STARTUPINFOA si; PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  char cmd[32768]; strncpy(cmd, cmdline, sizeof(cmd)-1); cmd[sizeof(cmd)-1]='\0';
  if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)){
    return -1;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 0; GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
  return (int)code;
#else
  int rc = system(cmdline);
  if (rc == -1) return -1;
  if (WIFEXITED(rc)) return WEXITSTATUS(rc);
  return -1;
#endif
}

int path_abs(const char* in, char* out, size_t outsz){
#ifdef _WIN32
  wchar_t wIn[PATH_MAX], wOut[PATH_MAX];
  MultiByteToWideChar(CP_UTF8,0,in,-1,wIn,PATH_MAX);
  DWORD n = GetFullPathNameW(wIn, PATH_MAX, wOut, NULL);
  if (n==0 || n>=PATH_MAX) return -1;
  WideCharToMultiByte(CP_UTF8,0,wOut,-1,out,(int)outsz,NULL,NULL);
  return 0;
#else
  if (!realpath(in, out)) return -1;
  return 0;
#endif
}

void path_to_file_url(const char* abs, char* out, size_t outsz){
#ifdef _WIN32
  char tmp[PATH_MAX]; strncpy(tmp, abs, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
  for (char* p=tmp; *p; ++p){ if (*p=='\\') *p='/'; }
  snprintf(out, outsz, "file:///%s", tmp);
#else
  snprintf(out, outsz, "file://%s", abs);
#endif
}

int browse_file_or_url(const char* path_or_url){
#ifdef _WIN32
  return (int)(intptr_t)ShellExecuteA(NULL, "open", path_or_url, NULL, NULL, SW_SHOWNORMAL) > 32 ? 0 : -1;
#else
  char cmd[PATH_MAX + 64];
  snprintf(cmd, sizeof(cmd),
#ifdef __APPLE__
           "open '%s'",
#else
           "xdg-open '%s' >/dev/null 2>&1",
#endif
           path_or_url);
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

/*------------------------------ Extra FS helpers ----------------------------*/
/* Binary copy with parent dir creation. Returns 0 on success. */
int copy_file_binary(const char* src, const char* dst){
  if (!src || !dst) return -1;
  FILE* in = ueng_fopen(src, "rb");
  if (!in) return -1;
  if (mkpath_parent(dst) != 0){ fclose(in); return -1; }
  FILE* out = ueng_fopen(dst, "wb");
  if (!out){ fclose(in); return -1; }
  char buf[64 * 1024];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), in)) > 0){
    if (fwrite(buf, 1, n, out) != n){ fclose(in); fclose(out); return -1; }
  }
  fclose(in);
  fclose(out);
  return 0;
}

/* Ensure a directory is present and write an empty .gitkeep file into it. */
int write_gitkeep(const char* dir){
  if (!dir || !*dir) return -1;
  if (mkpath(dir) != 0) return -1;
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s%c.gitkeep", dir, PATH_SEP);
  /* Don't overwrite on purpose; empty file if absent */
  return write_text_file_if_absent(path, "");
}

/* Remove all immediate children of a directory (files/dirs), keep the dir. */
int clean_dir(const char* dir){
  if (!dir || !*dir) return -1;
  if (!dir_exists(dir)) return 0;
#ifdef _WIN32
  char pattern[PATH_MAX];
  snprintf(pattern, sizeof(pattern), "%s\*.*", dir);
  WIN32_FIND_DATAA f;
  HANDLE h = FindFirstFileA(pattern, &f);
  if (h == INVALID_HANDLE_VALUE) return 0;
  do{
    const char* name = f.cFileName;
    if (strcmp(name, ".")==0 || strcmp(name, "..")==0) continue;
    char p[PATH_MAX];
    snprintf(p, sizeof(p), "%s\%s", dir, name);
    if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
      /* Recurse, then remove the now-empty directory */
      clean_dir(p);
      RemoveDirectoryA(p);
    } else {
      /* Clear readonly if set */
      if (f.dwFileAttributes & FILE_ATTRIBUTE_READONLY){
        SetFileAttributesA(p, f.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
      }
      DeleteFileA(p);
    }
  } while (FindNextFileA(h, &f));
  FindClose(h);
  return 0;
#else
  DIR* d = opendir(dir);
  if (!d) return 0;
  struct dirent* ent;
  while ((ent = readdir(d))){
    const char* name = ent->d_name;
    if (strcmp(name,".")==0 || strcmp(name,"..")==0) continue;
    char p[PATH_MAX];
    snprintf(p, sizeof(p), "%s/%s", dir, name);
    struct stat st;
    if (stat(p, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)){
      clean_dir(p);
      rmdir(p);
    } else {
      unlink(p);
    }
  }
  closedir(d);
  return 0;
#endif
}

/*---------------------------- book.yaml parser -----------------------------*/
/* (Parsing helpers live in main.c; this file hosts cross-platform utilities.) */
/*------------------------------ End of file --------------------------------*/
