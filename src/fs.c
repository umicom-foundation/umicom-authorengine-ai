/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * Project: Core CLI + Library-Ready Refactor
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 *
 * LICENSE
 *   SPDX-License-Identifier: MIT
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <direct.h>
#else
  #include <dirent.h>
  #include <sys/stat.h>
  #include <unistd.h>
#endif

#include "ueng/common.h"
#include "ueng/fs.h"

/*-------------------------------- utilities --------------------------------*/

/* simple ASCII case-insensitive string equality */
static int ci_streq(const char* a, const char* b){
  if (!a || !b) return 0;
  while (*a && *b){
    unsigned char ca=(unsigned char)*a++;
    unsigned char cb=(unsigned char)*b++;
    if ((unsigned char)tolower(ca) != (unsigned char)tolower(cb)) return 0;
  }
  return *a == *b;
}

static int has_ext_ci(const char* path, const char* ext){
  const char* dot = strrchr(path, '.');
  if (!dot) return 0;
  return ci_streq(dot, ext);
}

/*--------------------------------- ingest ----------------------------------*/

#ifdef _WIN32
static int ingest_walk_win(const char* abs_dir, const char* rel_dir, StrList* out){
  char pattern[PATH_MAX];
  snprintf(pattern, sizeof(pattern), "%s\\*", abs_dir);

  WIN32_FIND_DATAA ffd;
  HANDLE h = FindFirstFileA(pattern, &ffd);
  if (h == INVALID_HANDLE_VALUE) return 0;

  do {
    const char* n = ffd.cFileName;
    if (strcmp(n,".")==0 || strcmp(n,"..")==0) continue;

    char abs_child[PATH_MAX];
    snprintf(abs_child, sizeof(abs_child), "%s\\%s", abs_dir, n);

    char rel_child[PATH_MAX];
    if (rel_dir && *rel_dir) snprintf(rel_child, sizeof(rel_child), "%s\\%s", rel_dir, n);
    else snprintf(rel_child, sizeof(rel_child), "%s", n);

    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
      (void)ingest_walk_win(abs_child, rel_child, out);
    }else{
      if (has_ext_ci(n,".md") || has_ext_ci(n,".markdown") || has_ext_ci(n,".txt") || has_ext_ci(n,".pdf")){
        (void)sl_push(out, rel_child);
      }
    }
  } while (FindNextFileA(h, &ffd));
  FindClose(h);
  return 0;
}
#else
static int ingest_walk_posix(const char* abs_dir, const char* rel_dir, StrList* out){
  DIR* d = opendir(abs_dir);
  if (!d) return 0;
  struct dirent* ent;
  while ((ent=readdir(d))){
    const char* n = ent->d_name;
    if (strcmp(n,".")==0 || strcmp(n,"..")==0) continue;

    char abs_child[PATH_MAX];
    snprintf(abs_child, sizeof(abs_child), "%s/%s", abs_dir, n);

    char rel_child[PATH_MAX];
    if (rel_dir && *rel_dir) snprintf(rel_child, sizeof(rel_child), "%s/%s", rel_dir, n);
    else snprintf(rel_child, sizeof(rel_child), "%s", n);

    struct stat st;
    if (stat(abs_child, &st) == 0 && S_ISDIR(st.st_mode)){
      (void)ingest_walk_posix(abs_child, rel_child, out);
    }else{
      if (has_ext_ci(n,".md") || has_ext_ci(n,".markdown") || has_ext_ci(n,".txt") || has_ext_ci(n,".pdf")){
        (void)sl_push(out, rel_child);
      }
    }
  }
  closedir(d);
  return 0;
}
#endif

int ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out){
#ifdef _WIN32
  return ingest_walk_win(abs_dir, rel_dir, out);
#else
  return ingest_walk_posix(abs_dir, rel_dir, out);
#endif
}

/*-------------------------------- normalize --------------------------------*/

int normalize_chapters(const char* dropzone){
  /* Mirror *.md files from dropzone into workspace/chapters, flat for now. */
  char dest_dir[PATH_MAX];
  snprintf(dest_dir, sizeof(dest_dir), "workspace%cchapters", PATH_SEP);
  (void)mkpath(dest_dir);

  StrList files; sl_init(&files);
  if (ingest_walk(dropzone, NULL, &files) != 0){ sl_free(&files); return -1; }

  int copied = 0;
  for (size_t i=0; i<files.count; ++i){
    const char* rel = files.items[i];
    const char* base = strrchr(rel, PATH_SEP);
#ifdef _WIN32
    if (!base) base = strrchr(rel, '/');
#endif
    base = base ? base+1 : rel;

    if (!(has_ext_ci(base,".md") || has_ext_ci(base,".markdown"))) continue;

    char src[PATH_MAX]; snprintf(src, sizeof(src), "%s%c%s", dropzone, PATH_SEP, rel);
#ifdef _WIN32
    for (char* p=src; *p; ++p) if (*p=='/') *p='\\';
#endif

    char dst[PATH_MAX]; snprintf(dst, sizeof(dst), "%s%c%s", dest_dir, PATH_SEP, base);
    if (copy_file_binary(src, dst) == 0) copied++;
  }
  sl_free(&files);
  printf("[normalize] copied %d chapter files into %s\n", copied, dest_dir);
  return 0;
}

/*------------------------------ generators ---------------------------------*/

int generate_toc_md(const char* book_title){
  (void)mkpath("workspace");
  (void)mkpath("workspace/chapters");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%cchapters%c_toc.md", PATH_SEP, PATH_SEP);
  char buf[512];
  snprintf(buf, sizeof(buf),
           "# Table of Contents\n\n"
           "- Introduction\n"
           "- Chapter 1\n"
           "- Chapter 2\n");
  return write_text_file_if_absent(path, buf);
}

int generate_frontmatter_md(const char* title, const char* author){
  (void)mkpath("workspace/chapters");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%cchapters%c_frontmatter.md", PATH_SEP, PATH_SEP);
  char buf[512];
  snprintf(buf, sizeof(buf),
           "---\n"
           "title: \"%s\"\n"
           "author: \"%s\"\n"
           "lang: en-GB\n"
           "---\n\n"
           "_This is autogenerated front matter._\n",
           title ? title : "Untitled",
           author ? author : "Unknown");
  return write_text_file_if_absent(path, buf);
}

int generate_acknowledgements_md(const char* author){
  (void)mkpath("workspace/chapters");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%cchapters%cacknowledgements.md", PATH_SEP, PATH_SEP);
  char buf[512];
  snprintf(buf, sizeof(buf),
           "# Acknowledgements\n\n"
           "Thanks to %s, contributors, and readers.\n",
           author ? author : "the author");
  return write_text_file_if_absent(path, buf);
}

int generate_cover_svg(const char* title, const char* author, const char* slug){
  (void)mkpath("workspace");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%ccover.svg", PATH_SEP);
  char svg[4096];
  snprintf(svg, sizeof(svg),
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1200\" height=\"1600\">\n"
    "  <rect x=\"0\" y=\"0\" width=\"1200\" height=\"1600\" fill=\"#f3f4f6\"/>\n"
    "  <text x=\"600\" y=\"700\" font-family=\"Arial\" font-size=\"64\" text-anchor=\"middle\">%s</text>\n"
    "  <text x=\"600\" y=\"780\" font-family=\"Arial\" font-size=\"28\" text-anchor=\"middle\">%s</text>\n"
    "  <text x=\"600\" y=\"860\" font-family=\"Arial\" font-size=\"20\" text-anchor=\"middle\" fill=\"#555\">%s</text>\n"
    "</svg>\n",
    title ? title : "Untitled",
    author ? author : "Unknown",
    slug ? slug : "untitled");
  return write_text_file_if_absent(path, svg);
}

int generate_frontcover_md(const char* title, const char* author, const char* slug){
  (void)title; (void)author; (void)slug;
  (void)mkpath("workspace/chapters");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%cchapters%cfrontcover.md", PATH_SEP, PATH_SEP);
  const char* md = "# Cover\n\n![](../cover.svg)\n";
  return write_text_file_if_absent(path, md);
}

/* Pack workspace/chapters (any .md) -> workspace/book-draft.md (very simple order) */
static int concat_md_dir(const char* dir, FILE* out){
#ifdef _WIN32
  char pattern[PATH_MAX]; snprintf(pattern, sizeof(pattern), "%s\\*.md", dir);
  WIN32_FIND_DATAA f; HANDLE h = FindFirstFileA(pattern, &f);
  if (h == INVALID_HANDLE_VALUE) return 0;
  do{
    char p[PATH_MAX]; snprintf(p, sizeof(p), "%s\\%s", dir, f.cFileName);
    FILE* in = ueng_fopen(p, "rb"); if (!in) continue;
    fprintf(out, "\n\n<!-- %s -->\n\n", f.cFileName);
    char buf[65536]; size_t nrd;
    while ((nrd=fread(buf,1,sizeof(buf),in))>0) fwrite(buf,1,nrd,out);
    fclose(in);
  }while(FindNextFileA(h, &f));
  FindClose(h);
#else
  DIR* d = opendir(dir); if (!d) return 0;
  struct dirent* ent;
  while ((ent=readdir(d))){
    const char* n = ent->d_name;
    size_t ln = strlen(n);
    if (ln < 4) continue;
    if (strcmp(n + ln - 3, ".md") != 0) continue;
    char p[PATH_MAX]; snprintf(p, sizeof(p), "%s/%s", dir, n);
    FILE* in = ueng_fopen(p, "rb"); if (!in) continue;
    fprintf(out, "\n\n<!-- %s -->\n\n", n);
    char buf[65536]; size_t nrd;
    while ((nrd=fread(buf,1,sizeof(buf),in))>0) fwrite(buf,1,nrd,out);
    fclose(in);
  }
  closedir(d);
#endif
  return 0;
}

int pack_book_draft(const char* title, const char* outputs_root, int* out_has_draft){
  (void)outputs_root;
  (void)mkpath("workspace");
  char draft[PATH_MAX]; snprintf(draft, sizeof(draft), "workspace%cbook-draft.md", PATH_SEP);

  FILE* out = ueng_fopen(draft, "wb");
  if (!out){ fprintf(stderr,"[pack] ERROR: cannot open %s\n", draft); return -1; }

  fprintf(out, "# %s\n\n", title ? title : "Untitled");
  fprintf(out, "_Autogenerated draft._\n");

  (void)concat_md_dir("workspace/chapters", out);

  fclose(out);
  if (out_has_draft) *out_has_draft = file_exists(draft);
  if (out_has_draft && *out_has_draft) printf("[pack] wrote: %s\n", draft);
  return 0;
}

int copy_theme_into_html_dir(const char* html_dir, char* out_rel_css, size_t outsz){
  (void)mkpath(html_dir);
  char css[PATH_MAX]; snprintf(css, sizeof(css), "%s%cstyle.css", html_dir, PATH_SEP);
  const char* content =
    "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,Cantarell,Noto Sans,sans-serif;max-width:820px;margin:40px auto;padding:0 16px;}\n"
    "h1,h2,h3{line-height:1.2}\n"
    "img{max-width:100%%}\n"
    "code,pre{font-family:ui-monospace,Consolas,Menlo,monospace}\n";
  if (write_text_file_if_absent(css, content) == 0){
    if (out_rel_css && outsz) snprintf(out_rel_css, outsz, "style.css");
    return 0;
  }
  return -1;
}

int write_site_index(const char* site_dir,
                     const char* title,
                     const char* author,
                     const char* slug,
                     const char* stamp,
                     int has_cover,
                     int has_draft){
  (void)mkpath(site_dir);
  char html[PATH_MAX]; snprintf(html, sizeof(html), "%s%cindex.html", site_dir, PATH_SEP);
  char buf[4096];

  char book_html_rel[64]; snprintf(book_html_rel, sizeof(book_html_rel), "../html/book.html");

  int n = snprintf(buf, sizeof(buf),
      "<!doctype html>\n<html><head>\n"
      "<meta charset=\"utf-8\"/>\n"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>\n"
      "<title>%s</title>\n"
      "<link rel=\"stylesheet\" href=\"../html/style.css\"/>\n"
      "</head><body>\n"
      "<main>\n"
      "<h1>%s</h1>\n"
      "<p><em>by %s</em></p>\n"
      "<p><small>Slug: %s &middot; Built: %s</small></p>\n",
      title?title:"Untitled",
      title?title:"Untitled",
      author?author:"Unknown",
      slug?slug:"untitled",
      stamp?stamp:"");
  if (n<0 || n >= (int)sizeof(buf)) return -1;

  size_t used = (size_t)n;

  if (has_cover){
    n = snprintf(buf+used, sizeof(buf)-used,
      "<p><img src=\"cover.svg\" alt=\"Cover\"/></p>\n");
    if (n<0) return -1; used += (size_t)n;
  }
  if (has_draft){
    n = snprintf(buf+used, sizeof(buf)-used,
      "<p><a href=\"%s\">Read HTML Draft</a></p>\n",
      book_html_rel);
    if (n<0) return -1; used += (size_t)n;
  }

  n = snprintf(buf+used, sizeof(buf)-used,
      "<p>Generated by Umicom AuthorEngine AI.</p>\n"
      "</main>\n</body></html>\n");
  if (n<0) return -1;

  return write_file(html, buf);
}
