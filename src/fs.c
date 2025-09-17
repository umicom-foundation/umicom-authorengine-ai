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
#include "include/ueng/fs.h"

#ifdef _WIN32
  #include <windows.h>
int ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out) {
  int patn=snprintf(NULL,0,"%s\\*",abs_dir); if(patn<0) return -1;
  char* pattern=(char*)malloc((size_t)patn+1); if(!pattern) return -1;
  snprintf(pattern,(size_t)patn+1,"%s\\*",abs_dir);
  WIN32_FIND_DATAA ffd; HANDLE h=FindFirstFileA(pattern,&ffd);
  if (h==INVALID_HANDLE_VALUE) { free(pattern); return 0; }
  int rc=0;
  do {
    const char* name=ffd.cFileName;
    if (!strcmp(name,".")||!strcmp(name,"..")) continue;
    int need_abs=snprintf(NULL,0,"%s\\%s",abs_dir,name);
    char* child_abs=(char*)malloc((size_t)need_abs+1); if(!child_abs) { rc=-1; break; }
    snprintf(child_abs,(size_t)need_abs+1,"%s\\%s",abs_dir,name);

    int need_rel = rel_dir ? snprintf(NULL,0,"%s/%s",rel_dir,name) : (int)strlen(name);
    char* child_rel=(char*)malloc((size_t)need_rel+1); if(!child_rel) { free(child_abs); rc=-1; break; }
    if (rel_dir) snprintf(child_rel,(size_t)need_rel+1,"%s/%s",rel_dir,name);
    else         snprintf(child_rel,(size_t)need_rel+1,"%s",name);

    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      rc = ingest_walk(child_abs, child_rel, out);
    } else {
      rel_normalize(child_rel);
      if (sl_push(out, child_rel)!=0) rc=-1;
    }
    free(child_abs); free(child_rel);
    if (rc!=0) break;
  } while (FindNextFileA(h,&ffd));
  FindClose(h); free(pattern); return rc;
}
#else
  #include <dirent.h>
  #include <sys/stat.h>
int ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out) {
  DIR* d=opendir(abs_dir); if(!d) return 0; int rc=0; struct dirent* ent;
  while((ent=readdir(d))) {
    const char* name=ent->d_name;
    if (!strcmp(name,".")||!strcmp(name,"..")) continue;
    int need_abs=snprintf(NULL,0,"%s/%s",abs_dir,name);
    char* child_abs=(char*)malloc((size_t)need_abs+1); if(!child_abs) { rc=-1; break; }
    snprintf(child_abs,(size_t)need_abs+1,"%s/%s",abs_dir,name);

    int need_rel = rel_dir ? snprintf(NULL,0,"%s/%s",rel_dir,name) : (int)strlen(name);
    char* child_rel=(char*)malloc((size_t)need_rel+1); if(!child_rel) { free(child_abs); rc=-1; break; }
    if (rel_dir) snprintf(child_rel,(size_t)need_rel+1,"%s/%s",rel_dir,name);
    else         snprintf(child_rel,(size_t)need_rel+1,"%s",name);

    struct stat st; if (lstat(child_abs,&st)==0) {
      if (S_ISDIR(st.st_mode)) rc = ingest_walk(child_abs, child_rel, out);
      else if (S_ISREG(st.st_mode)) { rel_normalize(child_rel); if (sl_push(out, child_rel)!=0) rc=-1; }
    }
    free(child_abs); free(child_rel);
    if (rc!=0) break;
  }
  closedir(d); return rc;
}
#endif
