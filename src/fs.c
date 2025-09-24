/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: src/fs.c
 * Purpose: Filesystem helpers specific to uaengine (book packing, site files)
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * License: MIT
 *---------------------------------------------------------------------------*/
#include "ueng/fs.h"
#include "ueng/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dirent.h>
  #include <sys/stat.h>
  #include <sys/types.h>
#endif

/* ---- simple content writers used by `build` --------------------------------
   These functions create minimal starter files so a new project has something
   to look at immediately after `uaengine init` + `uaengine build`. */

int generate_toc_md(const char* title){
  (void)mkpath("workspace/chapters");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%cchapters%c_toc.md", PATH_SEP, PATH_SEP);
  char buf[256];
  snprintf(buf, sizeof(buf),
           "# Table of Contents\n\n"
           "_Auto-generated for %s._\n", title ? title : "Untitled");
  return write_text_file_if_absent(path, buf);
}

int generate_frontmatter_md(const char* title, const char* author){
  (void)mkpath("workspace/chapters");
  char path[PATH_MAX]; snprintf(path, sizeof(path), "workspace%cchapters%c_frontmatter.md", PATH_SEP, PATH_SEP);
  char buf[512];
  snprintf(buf, sizeof(buf),
           "# %s\n\n_by %s_\n\n"
           "_Front matter._\n",
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
  /* Very simple SVG to ensure we always have a cover to show. */
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

/* ---- pack book-draft.md ----------------------------------------------------
   Concatenate all *.md files in workspace/chapters into a single book-draft.md.
   We add HTML comments between files to make debugging easier. */

static int concat_md_dir(const char* dir, FILE* out){
#ifdef _WIN32
  char pattern[PATH_MAX]; snprintf(pattern, sizeof(pattern), "%s\\*.md", dir);
  WIN32_FIND_DATAA f; HANDLE h = FindFirstFileA(pattern, &f);
  if (h == INVALID_HANDLE_VALUE) return 0; /* nothing to do is fine */
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
  (void)outputs_root; /* draft always under workspace/ */
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

/* ---- site helpers ----------------------------------------------------------
   copy_theme_into_html_dir: ensures html/style.css exists.
   write_site_index: creates site/index.html that links to html/book.html. */

int copy_theme_into_html_dir(const char* html_dir, char* out_rel_css, size_t outsz){
  (void)mkpath(html_dir);
  char css[PATH_MAX]; snprintf(css, sizeof(css), "%s%cstyle.css", html_dir, PATH_SEP);
  /* Minimal CSS for a clean read; projects can override later. */
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

  const char* css_rel = "../html/style.css";
  const char* book_html_rel = "../html/book.html";

  int n = snprintf(buf, sizeof(buf),
      "<!doctype html>\n<html><head>\n"
      "<meta charset=\"utf-8\"/>\n"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>\n"
      "<title>%s</title>\n"
      "<link rel=\"stylesheet\" href=\"%s\"/>\n"
      "</head><body>\n"
      "<main>\n"
      "<h1>%s</h1>\n"
      "<p><em>by %s</em></p>\n"
      "<p><small>Slug: %s &middot; Built: %s</small></p>\n",
      title?title:"Untitled", css_rel,
      title?title:"Untitled",
      author?author:"Unknown",
      slug?slug:"untitled",
      stamp?stamp:"");
  if (n<0 || n >= (int)sizeof(buf)) return -1;

  size_t used = (size_t)n;

  if (has_cover){
    n = snprintf(buf+used, sizeof(buf)-used, "<p><img src=\"cover.svg\" alt=\"Cover\"/></p>\n");
    if (n<0) return -1; used += (size_t)n;
  }
  if (has_draft){
    n = snprintf(buf+used, sizeof(buf)-used, "<p><a href=\"%s\">Read HTML Draft</a></p>\n", book_html_rel);
    if (n<0) return -1; used += (size_t)n;
  }

  n = snprintf(buf+used, sizeof(buf)-used,
      "<p>Generated by Umicom AuthorEngine AI.</p>\n"
      "</main>\n</body></html>\n");
  if (n<0) return -1;

  /* Minimal, surgical change here: use the function that is implemented in common.c */
  return write_text_file(html, buf);
}
/*------------------------------ Path helpers -------------------------------*/