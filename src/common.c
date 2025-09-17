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
#include "include/ueng/common.h"

#ifndef _WIN32
  #ifndef FOK
    #define FOK F_OK
  #endif
#endif

/*---------------------------- StrList --------------------------------------*/
void sl_init(StrList* sl) { sl->items=NULL; sl->count=0; sl->cap=0; }
void sl_free(StrList* sl) {
  if (!sl) return;
  for (size_t i=0;i<sl->count;i++) free(sl->items[i]);
  free(sl->items); sl->items=NULL; sl->count=sl->cap=0;
}
int sl_push(StrList* sl, const char* s) {
  if (sl->count == sl->cap) {
    size_t ncap = sl->cap ? sl->cap*2 : 16;
    char** p = (char**)realloc(sl->items, ncap*sizeof(char*));
    if (!p) return -1;
    sl->items=p; sl->cap=ncap;
  }
  size_t n=strlen(s);
  char* dup=(char*)malloc(n+1); if(!dup) return -1;
  memcpy(dup,s,n+1);
  sl->items[sl->count++]=dup;
  return 0;
}

/*---------------------------- FS utils -------------------------------------*/
int helper_exists_file(const char* path) {
  #ifdef _WIN32
    return (access(path, 0) == ACCESS_EXISTS) ? 1 : 0;
  #else
    return (access(path, FOK) == 0) ? 1 : 0;
  #endif
}
FILE* ueng_fopen(const char* path, const char* mode) {
  #ifdef _WIN32
    FILE* f=NULL;
    return (fopen_s(&f, path, mode)==0)? f : NULL;
  #else
    return fopen(path, mode);
  #endif
}
int mkpath(const char* path) {
  if (!path || !*path) return 0;
  size_t len = strlen(path);
  char* buf = (char*)malloc(len+1); if(!buf) return -1;
  memcpy(buf, path, len); buf[len]='\0';
  #ifdef _WIN32
    for (size_t i=0;i<len;i++) if (buf[i]=='/') buf[i]='\\';
  #endif
  for (size_t i=1;i<len;i++) {
    if (buf[i]==PATH_SEP) {
      char saved = buf[i]; buf[i]='\0';
      if (!helper_exists_file(buf)) { if (ueng_make_dir(buf)!=0 && errno!=EEXIST) { /* best-effort */ } }
      buf[i]=saved;
    }
  }
  if (!helper_exists_file(buf)) { if (ueng_make_dir(buf)!=0 && errno!=EEXIST) { free(buf); return -1; } }
  free(buf); return 0;
}
int write_file(const char* path, const char* content) {
  FILE* f = ueng_fopen(path, "wb"); if(!f) return -1;
  if (content && *content) fputs(content, f);
  fclose(f); return 0;
}
int write_text_file_if_absent(const char* path, const char* content) {
  if (helper_exists_file(path)) return 0;
  return write_file(path, content);
}
int append_file(FILE* dst, const char* src_path) {
  FILE* src = ueng_fopen(src_path, "rb"); if(!src) return -1;
  char buf[64*1024]; size_t rd;
  while ((rd=fread(buf,1,sizeof(buf),src))>0) { if (fwrite(buf,1,rd,dst)!=rd) { fclose(src); return -1; } }
  fclose(src); return 0;
}
static int ensure_parent_dir(const char* filepath) {
  const char* last_sep=NULL;
  for (const char* p=filepath; *p; ++p) if (*p==PATH_SEP) last_sep=p;
  if (!last_sep) return 0;
  size_t n=(size_t)(last_sep-filepath);
  char* dir=(char*)malloc(n+1); if(!dir) return -1;
  memcpy(dir, filepath, n); dir[n]='\0';
  int rc = mkpath(dir); free(dir); return rc;
}
int copy_file_binary(const char* src, const char* dst) {
  FILE* in=ueng_fopen(src,"rb"); if(!in) return -1;
  if (ensure_parent_dir(dst)!=0) { fclose(in); return -1; }
  FILE* out=ueng_fopen(dst,"wb"); if(!out) { fclose(in); return -1; }
  char buf[64*1024]; size_t rd;
  while((rd=fread(buf,1,sizeof(buf),in))>0) { if (fwrite(buf,1,rd,out)!=rd) { fclose(in); fclose(out); return -1; } }
  fclose(in); fclose(out); return 0;
}

#ifdef _WIN32
int clean_dir(const char* dir) {
  if (!helper_exists_file(dir)) return 0;
  int patn = snprintf(NULL,0,"%s\\*",dir); if(patn<0) return -1;
  char* pattern=(char*)malloc((size_t)patn+1); if(!pattern) return -1;
  snprintf(pattern,(size_t)patn+1,"%s\\*",dir);
  WIN32_FIND_DATAA ffd; HANDLE h=FindFirstFileA(pattern,&ffd);
  if (h==INVALID_HANDLE_VALUE) { free(pattern); return 0; }
  int rc=0;
  do {
    const char* name=ffd.cFileName;
    if (!strcmp(name,".")||!strcmp(name,"..")) continue;
    int need=snprintf(NULL,0,"%s\\%s",dir,name);
    char* child=(char*)malloc((size_t)need+1); if(!child) { rc=-1; break; }
    snprintf(child,(size_t)need+1,"%s\\%s",dir,name);
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (clean_dir(child)!=0) rc=-1;
      SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
      if (!RemoveDirectoryA(child)) rc=-1;
    } else {
      SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
      if (!DeleteFileA(child)) rc=-1;
    }
    free(child);
  } while (FindNextFileA(h,&ffd));
  FindClose(h); free(pattern); return rc;
}
#else
#include <dirent.h>
int clean_dir(const char* dir) {
  if (!helper_exists_file(dir)) return 0;
  DIR* d=opendir(dir); if(!d) return 0; int rc=0;
  struct dirent* ent;
  while ((ent=readdir(d))) {
    const char* name=ent->d_name;
    if (!strcmp(name,".")||!strcmp(name,"..")) continue;
    int need=snprintf(NULL,0,"%s/%s",dir,name);
    char* child=(char*)malloc((size_t)need+1); if(!child) { rc=-1; break; }
    snprintf(child,(size_t)need+1,"%s/%s",dir,name);
    struct stat st; if (lstat(child,&st)==0) {
      if (S_ISDIR(st.st_mode)) { if (clean_dir(child)!=0) rc=-1; if (rmdir(child)!=0) rc=-1; }
      else { if (unlink(child)!=0) rc=-1; }
    }
    free(child);
  }
  closedir(d); return rc;
}
#endif

/*---------------------------- Strings --------------------------------------*/
char* ltrim(char* s) { while (*s && isspace((unsigned char)*s)) s++; return s; }
void  rtrim(char* s) { size_t n=strlen(s); while (n>0 && isspace((unsigned char)s[n-1])) s[--n]='\0'; }
void  unquote(char* s) {
  size_t n=strlen(s);
  if (n>=2) {
    char a=s[0], b=s[n-1];
    if ((a=='"'&&b=='"')||(a=='\''&&b=='\'')) { memmove(s,s+1,n-2); s[n-2]='\0'; }
  }
}

int ci_cmp(const char* a, const char* b) {
  for(;;) {
    unsigned char ca=(unsigned char)tolower(*a);
    unsigned char cb=(unsigned char)tolower(*b);
    if (ca<cb) return -1; if (ca>cb) return 1;
    if (ca==0 && cb==0) return 0; a++; b++;
  }
}
int natural_ci_cmp(const char* a, const char* b) {
  size_t i=0,j=0;
  for(;;) {
    unsigned char ca=(unsigned char)a[i];
    unsigned char cb=(unsigned char)b[j];
    if (ca=='\0'&&cb=='\0') return 0;
    if (isdigit(ca)&&isdigit(cb)) {
      unsigned long long va=0,vb=0; size_t ia=i,jb=j;
      while(isdigit((unsigned char)a[ia])){ va=va*10ULL+(unsigned)(a[ia]-'0'); ia++; }
      while(isdigit((unsigned char)b[jb])){ vb=vb*10ULL+(unsigned)(b[jb]-'0'); jb++; }
      if (va<vb) return -1; if (va>vb) return 1; i=ia; j=jb; continue;
    }
    unsigned char ta=(unsigned char)tolower(ca);
    unsigned char tb=(unsigned char)tolower(cb);
    if (ta<tb) return -1; if (ta>tb) return 1;
    if (ca!='\0') i++; if (cb!='\0') j++;
  }
}
int endswith_ic(const char* s, const char* suffix) {
  size_t ns=strlen(s), nx=strlen(suffix); if (nx>ns) return 0;
  return ci_cmp(s+(ns-nx), suffix)==0;
}
void rel_normalize(char* s) {
  #ifdef _WIN32
  for(char* p=s;*p;++p) if (*p=='\\') *p='/';
  #else
  (void)s;
  #endif
}
void rel_to_native_sep(char* s) {
  #ifdef _WIN32
  for(char* p=s;*p;++p) if (*p=='/') *p='\\';
  #else
  (void)s;
  #endif
}

/*---------------------------- Time / naming --------------------------------*/
void build_date_utc(char* out, size_t outsz) {
  time_t now=time(NULL); struct tm g;
  #if defined(_WIN32)
    gmtime_s(&g,&now);
  #else
    gmtime_r(&now,&g);
  #endif
  strftime(out,outsz,"%Y-%m-%d",&g);
}
void build_timestamp_utc(char* out, size_t outsz) {
  time_t now=time(NULL); struct tm g;
  #if defined(_WIN32)
    gmtime_s(&g,&now);
  #else
    gmtime_r(&now,&g);
  #endif
  strftime(out,outsz,"%Y-%m-%dT%H-%M-%SZ",&g);
}
void slugify(const char* title, char* out, size_t outsz) {
  size_t j=0; int prev_dash=0;
  for (size_t i=0; title[i] && j+1<outsz; ++i) {
    unsigned char c=(unsigned char)title[i];
    if (isalnum(c)) { out[j++]=(char)tolower(c); prev_dash=0; }
    else { if(!prev_dash && j>0) { out[j++]='-'; prev_dash=1; } }
  }
  if (j>0 && out[j-1]=='-') j--;
  out[j]='\0';
  if (j==0) snprintf(out,outsz,"%s","untitled");
}

/*---------------------------- Tiny YAML ------------------------------------*/
int tiny_yaml_get(const char* filename, const char* key, char* out, size_t outsz) {
  FILE* f=ueng_fopen(filename,"rb"); if(!f) return -1;
  char line[1024]; size_t keylen=strlen(key); int found=0;
  while (fgets(line,(int)sizeof(line),f)) {
    char* s=line; while(*s && isspace((unsigned char)*s)) s++; rtrim(s);
    if (*s=='#'||*s=='\0') continue;
    if (strncmp(s,key,keylen)==0) {
      const char* p=s+keylen; while(*p==' '||*p=='\t') p++;
      if (*p==':') { p++; while(*p==' '||*p=='\t') p++; size_t n=strlen(p);
        if (n>=outsz) n=outsz-1; memcpy(out,p,n); out[n]='\0'; rtrim(out); unquote(out); found=1; break; }
    }
  }
  fclose(f); return found?0:1;
}
int tiny_yaml_get_bool(const char* filename, const char* key, int* out) {
  char buf[64]={0};
  int rc=tiny_yaml_get(filename,key,buf,sizeof(buf)); if(rc!=0) return rc;
  for(char* p=buf;*p;++p) *p=(char)tolower((unsigned char)*p);
  if (!strcmp(buf,"true")||!strcmp(buf,"yes")||!strcmp(buf,"on")||!strcmp(buf,"1")) { *out=1; return 0; }
  if (!strcmp(buf,"false")||!strcmp(buf,"no")||!strcmp(buf,"off")||!strcmp(buf,"0")) { *out=0; return 0; }
  *out=(buf[0]!='\0'); return 0;
}
