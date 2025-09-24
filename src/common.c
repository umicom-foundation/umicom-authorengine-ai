/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: src/common.c
 * Purpose: Platform shims, filesystem + string helpers, exec utilities
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * License: MIT
 *---------------------------------------------------------------------------*/
#include "ueng/common.h"

#ifdef _WIN32
  #include <Shlwapi.h>
#endif

/*---------------------------- string list -----------------------------------*/
void sl_init(StrList* sl){ sl->items=NULL; sl->count=0; sl->cap=0; }
void sl_free(StrList* sl){
  if (!sl) return;
  for (size_t i=0;i<sl->count;i++) free(sl->items[i]);
  free(sl->items); sl->items=NULL; sl->count=sl->cap=0;
}
int sl_push(StrList* sl, const char* s){
  if (sl->count == sl->cap){
    size_t ncap = sl->cap ? sl->cap*2 : 16;
    char **p = (char**)realloc(sl->items, ncap * sizeof(char*));
    if (!p) return -1;
    sl->items = p; sl->cap = ncap;
  }
  size_t n = strlen(s);
  char *dup = (char*)malloc(n+1);
  if (!dup) return -1;
  memcpy(dup, s, n+1);
  sl->items[sl->count++] = dup;
  return 0;
}

/* natural, case-insensitive compare for filenames like 2 < 10 */
static int natural_ci_cmp(const char* a, const char* b){
  size_t i=0,j=0;
  for(;;){
    unsigned char ca=(unsigned char)a[i];
    unsigned char cb=(unsigned char)b[j];
    if (ca=='\0' && cb=='\0') return 0;
    if (isdigit(ca) && isdigit(cb)){
      unsigned long long va=0,vb=0; size_t ia=i,jb=j;
      while (isdigit((unsigned char)a[ia])){ va=va*10ULL+(unsigned)(a[ia]-'0'); ia++; }
      while (isdigit((unsigned char)b[jb])){ vb=vb*10ULL+(unsigned)(b[jb]-'0'); jb++; }
      if (va<vb) return -1; if (va>vb) return 1;
      i=ia; j=jb; continue;
    }
    unsigned char ta=(unsigned char)tolower(ca);
    unsigned char tb=(unsigned char)tolower(cb);
    if (ta<tb) return -1; if (ta>tb) return 1;
    if (ca!='\0') i++; if (cb!='\0') j++;
  }
}
int qsort_nat_ci_cmp(const void* A, const void* B){
  const char* a = *(const char* const*)A;
  const char* b = *(const char* const*)B;
  return natural_ci_cmp(a,b);
}

/*-------------------------------- fs ----------------------------------------*/
int file_exists(const char* path){
#ifdef _WIN32
  return (access(path, 0) == ACCESS_EXISTS) ? 1 : 0;
#else
  return (access(path, F_OK) == 0) ? 1 : 0;
#endif
}

FILE* ueng_fopen(const char* path, const char* mode){
#ifdef _WIN32
  FILE* f=NULL;
  return (fopen_s(&f, path, mode) == 0) ? f : NULL;
#else
  return fopen(path, mode);
#endif
}

int write_text_file_if_absent(const char* path, const char* content){
  if (file_exists(path)) { printf("[init] skip (exists): %s\n", path); return 0; }
  FILE* f = ueng_fopen(path, "wb");
  if (!f){ fprintf(stderr,"[init] ERROR: cannot open '%s'\n", path); return -1; }
  if (content && *content) fputs(content, f);
  fclose(f); printf("[init] wrote: %s\n", path); return 0;
}

int write_file(const char* path, const char* content){
  FILE* f = ueng_fopen(path, "wb");
  if (!f){ fprintf(stderr,"[write] ERROR: cannot open '%s'\n", path); return -1; }
  if (content && *content) fputs(content, f);
  fclose(f); return 0;
}

static int ensure_parent_dir(const char* filepath){
  const char* last_sep=NULL;
  for (const char* p=filepath; *p; ++p) if (*p == PATH_SEP) last_sep=p;
  if (!last_sep) return 0;
  size_t n = (size_t)(last_sep - filepath);
  char* dir = (char*)malloc(n+1); if (!dir) return -1;
  memcpy(dir, filepath, n); dir[n]='\0';
  int rc = mkpath(dir);
  free(dir);
  return rc;
}

int copy_file_binary(const char* src, const char* dst){
  FILE* in = ueng_fopen(src, "rb");
  if (!in){ fprintf(stderr,"[copy] open src fail: %s\n", src); return -1; }
  if (ensure_parent_dir(dst) != 0){ fclose(in); fprintf(stderr,"[copy] mkpath fail for %s\n", dst); return -1; }
  FILE* out = ueng_fopen(dst, "wb");
  if (!out){ fclose(in); fprintf(stderr,"[copy] open dst fail: %s\n", dst); return -1; }
  char buf[64*1024];
  size_t rd;
  while ((rd=fread(buf,1,sizeof(buf),in))>0){
    size_t wr=fwrite(buf,1,rd,out);
    if (wr!=rd){ fclose(in); fclose(out); fprintf(stderr,"[copy] short write: %s\n", dst); return -1; }
  }
  fclose(in); fclose(out); return 0;
}

int write_gitkeep(const char* dir){
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s%c.gitkeep", dir, PATH_SEP);
  return write_text_file_if_absent(path, "");
}

/* mkdir -p */
int mkpath(const char* path){
  if (!path || !*path) return 0;
  size_t len = strlen(path);
  char *buf = (char*)malloc(len+1);
  if (!buf){ fprintf(stderr,"[mkpath] OOM\n"); return -1; }
  memcpy(buf, path, len); buf[len]='\0';
#ifdef _WIN32
  for (size_t i=0;i<len;++i) if (buf[i]=='/') buf[i]='\\';
#endif
  for (size_t i=1;i<len;++i){
    if (buf[i] == PATH_SEP){
      char saved = buf[i]; buf[i]='\0';
      if (!file_exists(buf)){
        if (make_dir(buf) != 0 && errno != EEXIST){
          /* ignore; may already exist due to race */
        }
      }
      buf[i]=saved;
    }
  }
  if (!file_exists(buf)){
    if (make_dir(buf) != 0 && errno != EEXIST){
      free(buf); return -1;
    }
  }
  free(buf); return 0;
}

#ifdef _WIN32
#include <windows.h>
static int clean_dir_win(const char* dir){
  if (!file_exists(dir)) return 0;
  int rc = 0;
  char pattern[PATH_MAX];
  snprintf(pattern, sizeof(pattern), "%s\\*", dir);
  WIN32_FIND_DATAA ffd;
  HANDLE h = FindFirstFileA(pattern, &ffd);
  if (h == INVALID_HANDLE_VALUE) return 0;
  do {
    const char* name = ffd.cFileName;
    if (strcmp(name,".")==0 || strcmp(name,"..")==0) continue;
    char child[PATH_MAX];
    snprintf(child, sizeof(child), "%s\\%s", dir, name);
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
      (void)clean_dir_win(child);
      SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
      if (!RemoveDirectoryA(child)) rc = -1;
    }else{
      SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
      if (!DeleteFileA(child)) rc = -1;
    }
  } while (FindNextFileA(h, &ffd));
  FindClose(h);
  return rc;
}
int clean_dir(const char* dir){ return clean_dir_win(dir); }
void ueng_console_utf8(void){ SetConsoleOutputCP(CP_UTF8); }
#else
#include <dirent.h>
#include <sys/stat.h>
int clean_dir(const char* dir){
  if (!file_exists(dir)) return 0;
  DIR* d = opendir(dir);
  if (!d) return 0;
  int rc = 0;
  struct dirent* ent;
  while ((ent = readdir(d))){
    const char* name = ent->d_name;
    if (strcmp(name,".")==0 || strcmp(name,"..")==0) continue;
    char child[PATH_MAX];
    snprintf(child, sizeof(child), "%s/%s", dir, name);
    struct stat st;
    if (lstat(child, &st) == 0){
      if (S_ISDIR(st.st_mode)){
        if (clean_dir(child) != 0) rc = -1;
        if (rmdir(child) != 0) rc = -1;
      }else{
        if (unlink(child) != 0) rc = -1;
      }
    }
  }
  closedir(d);
  return rc;
}
#endif

/*----------------------------- time/format ----------------------------------*/
void build_date_utc(char* out, size_t outsz){
  time_t now=time(NULL);
  struct tm g;
#if defined(_WIN32)
  gmtime_s(&g,&now);
#else
  gmtime_r(&now,&g);
#endif
  strftime(out,outsz,"%Y-%m-%d",&g);
}
void build_timestamp_utc(char* out, size_t outsz){
  time_t now=time(NULL);
  struct tm g;
#if defined(_WIN32)
  gmtime_s(&g,&now);
#else
  gmtime_r(&now,&g);
#endif
  strftime(out,outsz,"%Y-%m-%dT%H-%M-%SZ",&g);
}

char* ltrim(char* s){ while(*s && isspace((unsigned char)*s)) s++; return s; }
void  rtrim(char* s){
  size_t n = strlen(s);
  while (n>0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}
void  unquote(char* s){
  size_t n=strlen(s);
  if (n>=2){
    char a=s[0], b=s[n-1];
    if ((a=='"' && b=='"') || (a=='\'' && b=='\'')) { memmove(s,s+1,n-2); s[n-2]='\0'; }
  }
}

void slugify(const char* title, char* out, size_t outsz){
  size_t j=0; int prev_dash=0;
  for (size_t i=0; title[i] && j+1<outsz; ++i){
    unsigned char c=(unsigned char)title[i];
    if (isalnum(c)){ out[j++] = (char)tolower(c); prev_dash=0; }
    else { if (!prev_dash && j>0){ out[j++]='-'; prev_dash=1; } }
  }
  if (j>0 && out[j-1]=='-') j--;
  out[j]='\0';
  if (j==0) snprintf(out,outsz,"%s","untitled");
}

/*------------------------------ exec/helpers --------------------------------*/
int exec_cmd(const char* cmdline){
  printf("[exec] %s\n", cmdline);
  int rc = system(cmdline);
  if (rc != 0) fprintf(stderr,"[exec] command failed (rc=%d)\n", rc);
  return rc;
}
#ifdef _WIN32
int path_abs(const char* in, char* out, size_t outsz){
  DWORD n = GetFullPathNameA(in, (DWORD)outsz, out, NULL);
  return (n>0 && n<outsz) ? 0 : -1;
}
void path_to_file_url(const char* abs, char* out, size_t outsz){
  size_t j=0;
  const char* pfx="file:///";
  for (size_t i=0; pfx[i] && j+1<outsz; ++i) out[j++]=pfx[i];
  for (size_t i=0; abs[i] && j+1<outsz; ++i){
    char c = abs[i]; if (c=='\\') c='/'; out[j++]=c;
  }
  out[j]='\0';
}
#else
#include <limits.h>
#include <unistd.h>
int path_abs(const char* in, char* out, size_t outsz){
  char* r = realpath(in, out);
  return r ? 0 : -1;
}
void path_to_file_url(const char* abs, char* out, size_t outsz){
  snprintf(out, outsz, "file://%s", abs);
}
#endif
