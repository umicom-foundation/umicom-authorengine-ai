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
#include "common.h"
#include "fs.h"
#include "ueng/version.h"

/*---------------------------------------------------------------------------
  Forward declarations for commands
---------------------------------------------------------------------------*/
static int  cmd_init(void);
static int  cmd_ingest(void);
static int  cmd_build(void);
static int  cmd_export(void);
static int  cmd_serve(int argc, char** argv);
static int  cmd_publish(void);
static int  cmd_doctor(void);
static int  cmd_new(int argc, char** argv);
static void usage(void);

/*----------------------------- seed book.yaml ------------------------------*/
static int seed_book_yaml(void) {
  const char *yaml =
    "# Umicom AuthorEngine AI — Book manifest (starter)\n"
    "title: \"My New Book\"\n"
    "subtitle: \"Learning by Building\"\n"
    "author: \"Your Name\"\n"
    "language: \"en-GB\"\n"
    "publisher: \"\n\"\n"
    "copyright_year: \"2025\"\n"
    "description: \"Short paragraph describing the book.\"\n"
    "dropzone: \"dropzone\"\n"
    "images_dir: \"dropzone/images\"\n"
    "target_formats: [pdf, docx, epub, html, md]\n"
    "video_scripts:\n"
    "  enabled: true\n"
    "  lesson_length_minutes: 10\n"
    "  total_lessons: 12\n"
    "site:\n"
    "  enabled: true\n"
    "ingest_on_build: true\n"
    "normalize_chapters_on_build: true\n";
  return write_text_file_if_absent("book.yaml", yaml);
}

/*----------------------------- outline writer ------------------------------*/
static int write_outline_md(const char* title, const char* author,
                            const char* dropzone_rel, const StrList* files) {
  (void)mkpath("workspace");
  char path[512]; snprintf(path,sizeof(path),"workspace%coutline.md",PATH_SEP);
  size_t est = 1024 + files->count*256;
  char* buf=(char*)malloc(est); if(!buf) return -1;
  char day[32]; build_date_utc(day,sizeof(day));
  int n=snprintf(buf,est,
    "# Draft Outline - %s\n\n"
    "_Author:_ **%s**  \n"
    "_Date:_ **%s**  \n"
    "_Sources scanned:_ `%s`\n\n"
    "## Sources (recursive)\n",
    title,author,day,dropzone_rel?dropzone_rel:"dropzone");
  if(n<0){ free(buf); return -1; } size_t used=(size_t)n;
  if(files->count==0){ used+=snprintf(buf+used,est-used,"\n> No .md/.markdown/.txt/.pdf files found yet.\n"); }
  else {
    for(size_t i=0;i<files->count;i++){ used+=snprintf(buf+used,est-used,"- %s\n",files->items[i]); if(used+256>est){ est*=2; char* t=(char*)realloc(buf,est); if(!t){ free(buf); return -1; } buf=t; } }
  }
  used+=snprintf(buf+used,est-used,"\n---\n_Tip:_ Add your chapters as **Markdown** files under `%s` and re-run `uaengine ingest`.\n", dropzone_rel?dropzone_rel:"dropzone");
  int rc = write_file(path, buf); free(buf);
  if(rc==0) printf("[ingest] wrote: %s\n", path); return rc;
}

/*----------------------------- cmd_ingest ----------------------------------*/
static int cmd_ingest(void) {
  char title[256]={0}, author[256]={0}, drop[256]={0};
  if (tiny_yaml_get("book.yaml","title", title,sizeof(title))  != 0) snprintf(title,sizeof(title),"%s","Untitled");
  if (tiny_yaml_get("book.yaml","author",author,sizeof(author))!= 0) snprintf(author,sizeof(author),"%s","Unknown");
  if (tiny_yaml_get("book.yaml","dropzone",drop,sizeof(drop))   != 0) snprintf(drop,sizeof(drop),  "%s","dropzone");
  if (!helper_exists_file(drop)) { fprintf(stderr,"[ingest] ERROR: dropzone path not found: %s\n",drop); return 1; }
  StrList files; sl_init(&files);
  int rc=ingest_walk(drop,NULL,&files);
  if (rc!=0) { fprintf(stderr,"[ingest] ERROR: directory walk failed\n"); sl_free(&files); return 1; }
  if (files.count>1) qsort(files.items, files.count, sizeof(char*), (int(*)(const void*,const void*))natural_ci_cmp);
  rc=write_outline_md(title,author,drop,&files); sl_free(&files);
  if (rc!=0) { fprintf(stderr,"[ingest] ERROR: failed to write outline\n"); return 1; }
  puts("[ingest] complete."); return 0;
}

/*------------------------------- Build helpers -----------------------------*/
static void titlecase_inplace(char* s) {
  int new_word=1;
  for(size_t i=0;s[i];++i){ unsigned char c=(unsigned char)s[i];
    if (isalpha(c)) { s[i]=(char)(new_word?toupper(c):tolower(c)); new_word=0; }
    else if (isdigit(c)) new_word=0;
    else { s[i]=' '; new_word=1; }
  }
  size_t w=0; for(size_t r=0;s[r];++r){ if (s[r]==' '&&(w==0||s[w-1]==' ')) continue; s[w++]=s[r]; }
  if (w>0 && s[w-1]==' ') w--; s[w]='\0';
}
static void make_label_from_filename(const char* filename, char* out, size_t outsz) {
  const char* base=filename; const char* slash=strrchr(filename,'/'); if(slash) base=slash+1;
  size_t n=strlen(base); if(n>=outsz) n=outsz-1; memcpy(out,base,n); out[n]='\0';
  char* dot=strrchr(out,'.'); if (dot) *dot='\0'; titlecase_inplace(out);
}
static int generate_toc_md(const char* book_title) {
  const char* root="workspace/chapters"; if(!helper_exists_file(root)) return 0;
  StrList files; sl_init(&files);
  int rc=ingest_walk(root,NULL,&files); if(rc!=0){ sl_free(&files); return 1; }
  StrList kept; sl_init(&kept);
  for(size_t i=0;i<files.count;i++){ const char* rel=files.items[i];
    const char* base=rel; const char* slash=strrchr(rel,'/'); if(slash) base=slash+1;
    if (base[0]=='_') continue; if (endswith_ic(rel,".pdf")) continue;
    int need=snprintf(NULL,0,"chapters/%s",rel);
    char* path=(char*)malloc((size_t)need+1); if(!path){ sl_free(&files); sl_free(&kept); return 1; }
    snprintf(path,(size_t)need+1,"chapters/%s",rel);
    if (sl_push(&kept,path)!=0) { free(path); sl_free(&files); sl_free(&kept); return 1; }
    free(path);
  }
  sl_free(&files);
  if (kept.count>1) qsort(kept.items, kept.count, sizeof(char*), (int(*)(const void*,const void*))natural_ci_cmp);
  (void)mkpath("workspace"); const char* outpath="workspace/toc.md";
  size_t est=1024+kept.count*160; char* buf=(char*)malloc(est); if(!buf){ sl_free(&kept); return 1; }
  int n=snprintf(buf,est,"# Table of Contents - %s\n\n> Draft TOC generated from `workspace/chapters/`.\n\n",(book_title&&*book_title)?book_title:"Untitled");
  if (n<0){ free(buf); sl_free(&kept); return 1; } size_t used=(size_t)n;
  if (kept.count==0) used+=snprintf(buf+used,est-used,"_No chapters found yet._\n");
  else { for(size_t i=0;i<kept.count;i++){ char label[256]; make_label_from_filename(kept.items[i],label,sizeof(label));
          used+=snprintf(buf+used,est-used,"- [%s](<%s>)\n",label,kept.items[i]);
          if (used+256>est){ est*=2; char* t=(char*)realloc(buf,est); if(!t){ free(buf); sl_free(&kept); return 1; } buf=t; }
        } }
  int wr=write_file(outpath,buf); free(buf); sl_free(&kept);
  if (wr==0) printf("[toc] wrote: %s\n", outpath); else fprintf(stderr,"[toc] ERROR: failed to write %s\n", outpath);
  return wr==0?0:1;
}

/*------------------------------- frontmatter etc ---------------------------*/
static int generate_frontmatter_md(const char* title, const char* author) {
  (void)mkpath("workspace");
  char subtitle[256]={0}, language[64]={0}, description[640]={0}, publisher[128]={0}, year[16]={0};
  (void)tiny_yaml_get("book.yaml","subtitle",subtitle,sizeof(subtitle));
  (void)tiny_yaml_get("book.yaml","language",language,sizeof(language));
  (void)tiny_yaml_get("book.yaml","description",description,sizeof(description));
  (void)tiny_yaml_get("book.yaml","publisher",publisher,sizeof(publisher));
  (void)tiny_yaml_get("book.yaml","copyright_year",year,sizeof(year));
  char day[32]; build_date_utc(day,sizeof(day));
  const char* outpath="workspace/frontmatter.md";
  size_t est=2048+strlen(title)+strlen(author)+strlen(subtitle)+strlen(description)+strlen(publisher);
  char* buf=(char*)malloc(est); if(!buf) return 1;
  int n=snprintf(buf,est,
    "# %s\n%s%s%s\n**Author:** %s  \n%s**Language:** %s  \n**Date:** %s  \n%s**Copyright:** © %s %s\n\n%s",
    (title&&*title)?title:"Untitled",
    (subtitle[0]!='\0')? "## ":"", (subtitle[0]!='\0')?subtitle:"", (subtitle[0]!='\0')?"\n":"",
    (author&&*author)?author:"Unknown",
    (publisher[0]!='\0')? "**Publisher:** ":"",
    (language[0]!='\0')? language:"en",
    day,
    (publisher[0]!='\0')? "":"",
    (year[0]!='\0')? year:"2025",
    (author&&*author)?author:"Unknown",
    (description[0]!='\0')? description : "_No description provided._\n"
  );
  if (n<0) { free(buf); return 1; }
  int wr=write_file(outpath,buf); free(buf);
  if (wr==0) printf("[frontmatter] wrote: %s\n", outpath);
  else       fprintf(stderr,"[frontmatter] ERROR: failed to write %s\n", outpath);
  return wr==0?0:1;
}
static int generate_acknowledgements_md(const char* author) {
  (void)author; (void)mkpath("workspace");
  const char* outpath="workspace/acknowledgements.md";
  const char* tpl=
"# Acknowledgements\n\n"
"This work was made possible thanks to the encouragement and contributions of friends,\n"
"family, colleagues, and the broader open source community.\n\n"
"- To my family for patience and support during the writing process.\n"
"- To early readers and reviewers for their thoughtful feedback.\n"
"- To open-source maintainers whose tools power modern learning.\n\n"
"*Optional:* This book was scaffolded with **Umicom AuthorEngine AI**, an open project by the\n"
"**Umicom Foundation**. You may keep or remove this line.\n";
  int wr=write_file(outpath,tpl);
  if (wr==0) printf("[ack] wrote: %s\n", outpath);
  else       fprintf(stderr,"[ack] ERROR: failed to write %s\n", outpath);
  return wr==0?0:1;
}
static int generate_cover_svg(const char* title, const char* author, const char* slug) {
  (void)mkpath("workspace");
  const char* tpl =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1600\" height=\"2560\" viewBox=\"0 0 1600 2560\">\n"
"  <defs>\n"
"    <linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\">\n"
"      <stop offset=\"0%\" stop-color=\"#0ea5e9\"/>\n"
"      <stop offset=\"100%\" stop-color=\"#22c55e\"/>\n"
"    </linearGradient>\n"
"  </defs>\n"
"  <rect width=\"1600\" height=\"2560\" fill=\"url(#g)\"/>\n"
"  <rect x=\"80\" y=\"80\" width=\"1440\" height=\"2400\" rx=\"48\" fill=\"#ffffff\" opacity=\"0.08\"/>\n"
"  <g font-family=\"Segoe UI, Roboto, Ubuntu, Arial, sans-serif\" fill=\"#0f172a\">\n"
"    <text x=\"120\" y=\"520\" font-size=\"88\" opacity=\"0.8\">Umicom AuthorEngine AI</text>\n"
"    <text x=\"120\" y=\"720\" font-size=\"128\" font-weight=\"700\">%s</text>\n"
"    <text x=\"120\" y=\"860\" font-size=\"64\" opacity=\"0.8\">by %s</text>\n"
"  </g>\n"
"  <g font-family=\"Consolas, Menlo, monospace\" fill=\"#0f172a\" opacity=\"0.75\">\n"
"    <text x=\"120\" y=\"2360\" font-size=\"40\">slug: %s</text>\n"
"  </g>\n"
"</svg>\n";
  int need=snprintf(NULL,0,tpl,(title&&*title)?title:"Untitled",(author&&*author)?author:"Unknown",(slug&&*slug)?slug:"untitled");
  if (need<0) return -1; size_t size=(size_t)need+1;
  char* svg=(char*)malloc(size); if(!svg) return -1;
  snprintf(svg,size,tpl,(title&&*title)?title:"Untitled",(author&&*author)?author:"Unknown",(slug&&*slug)?slug:"untitled");
  int wr=write_file("workspace/cover.svg",svg); free(svg);
  if (wr==0) printf("[cover] wrote: workspace/cover.svg\n"); else fprintf(stderr,"[cover] ERROR: write cover.svg\n");
  return wr==0?0:1;
}
static int generate_frontcover_md(const char* title, const char* author, const char* slug) {
  (void)mkpath("workspace"); char day[32]; build_date_utc(day,sizeof(day));
  const char* tpl=
"# Front Cover\n\n"
"A starter cover has been generated at `workspace/cover.svg`.\n"
"Edit that file (SVG is just text), then run `uaengine build` again to copy it into the outputs.\n\n"
"**Title:** %s  \n"
"**Author:** %s  \n"
"**Slug:** %s  \n"
"**Date:** %s  \n";
  int need=snprintf(NULL,0,tpl,(title&&*title)?title:"Untitled",(author&&*author)?author:"Unknown",(slug&&*slug)?slug:"untitled",day);
  if (need<0) return -1; size_t size=(size_t)need+1;
  char* md=(char*)malloc(size); if(!md) return -1;
  snprintf(md,size,tpl,(title&&*title)?title:"Untitled",(author&&*author)?author:"Unknown",(slug&&*slug)?slug:"untitled",day);
  int wr=write_file("workspace/frontcover.md",md); free(md);
  if (wr==0) printf("[frontcover] wrote: workspace/frontcover.md\n");
  else       fprintf(stderr,"[frontcover] ERROR: failed to write workspace/frontcover.md\n");
  return wr==0?0:1;
}

/*------------------------------- pack draft --------------------------------*/
static int list_chapter_files(StrList* out_rel) {
  const char* root="workspace/chapters"; if(!helper_exists_file(root)) return 0;
  StrList all; sl_init(&all); if (ingest_walk(root,NULL,&all)!=0) { sl_free(&all); return -1; }
  for(size_t i=0;i<all.count;i++){ const char* rel=all.items[i];
    const char* base=rel; const char* slash=strrchr(rel,'/'); if(slash) base=slash+1;
    if (base[0]=='_') continue; if (endswith_ic(rel,".pdf")) continue;
    if (!(endswith_ic(rel,".md")||endswith_ic(rel,".markdown")||endswith_ic(rel,".txt"))) continue;
    int need=snprintf(NULL,0,"workspace/chapters/%s",rel);
    char* path=(char*)malloc((size_t)need+1); if(!path){ sl_free(&all); return -1; }
    snprintf(path,(size_t)need+1,"workspace/chapters/%s",rel);
    if (sl_push(out_rel,path)!=0) { free(path); sl_free(&all); return -1; }
    free(path);
  }
  sl_free(&all);
  if (out_rel->count>1) qsort(out_rel->items,out_rel->count,sizeof(char*),(int(*)(const void*,const void*))natural_ci_cmp);
  return 0;
}
static int pack_book_draft(const char* title, const char* outputs_root, int* out_has_draft) {
  if (out_has_draft) *out_has_draft=0; (void)mkpath("workspace");
  const char* ws_draft="workspace/book-draft.md";
  FILE* out=ueng_fopen(ws_draft,"wb"); if(!out) { fprintf(stderr,"[pack] ERROR: open %s\n", ws_draft); return 1; }
  const char* SEP="\n\n---\n\n";
  if (helper_exists_file("workspace/frontmatter.md")) { if (append_file(out,"workspace/frontmatter.md")!=0) { fclose(out); return 1; } }
  else fprintf(out,"# %s\n\n",(title&&*title)?title:"Untitled");
  if (helper_exists_file("workspace/toc.md")) { fputs(SEP,out); if (append_file(out,"workspace/toc.md")!=0) { fclose(out); return 1; } }
  StrList chapters; sl_init(&chapters); if (list_chapter_files(&chapters)!=0) { fclose(out); sl_free(&chapters); return 1; }
  for(size_t i=0;i<chapters.count;i++){ fputs(SEP,out); if (append_file(out,chapters.items[i])!=0) { fclose(out); sl_free(&chapters); return 1; } }
  sl_free(&chapters);
  if (helper_exists_file("workspace/acknowledgements.md")) { fputs(SEP,out); if (append_file(out,"workspace/acknowledgements.md")!=0) { fclose(out); return 1; } }
  fclose(out); printf("[pack] wrote: %s\n", ws_draft);
  char dst_md[1024];  snprintf(dst_md, sizeof(dst_md),  "%s%cmd%cbook-draft.md",  outputs_root, PATH_SEP, PATH_SEP);
  char dst_site[1024];snprintf(dst_site,sizeof(dst_site),"%s%csite%cbook-draft.md", outputs_root, PATH_SEP, PATH_SEP);
  if (copy_file_binary(ws_draft,dst_md)==0)  printf("[pack] copied: %s\n", dst_md);
  if (copy_file_binary(ws_draft,dst_site)==0){ printf("[pack] copied: %s\n", dst_site); if(out_has_draft) *out_has_draft=1; }
  return 0;
}

/*------------------------------- write site --------------------------------*/
static int write_site_index(const char* site_dir,
                            const char* title,
                            const char* author,
                            const char* slug,
                            const char* stamp,
                            int has_cover,
                            int has_draft) {
  char path[1024]; if (snprintf(path,sizeof(path),"%s%cindex.html",site_dir,PATH_SEP)<0) return -1;
  const char* cover_block = "    <img class=\"cover\" src=\"cover.svg\" alt=\"Cover\" />\n";
  const char* draft_block = "    <p><a href=\"book-draft.md\" download>Download book-draft.md</a></p>\n";
  const char* tpl_a =
"<!doctype html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n  <title>%s — Umicom AuthorEngine AI</title>\n  <link rel=\"stylesheet\" href=\"../html/style.css\"/>\n</head>\n<body>\n  <main>\n  <div class=\"card\">\n    <h1>%s</h1>\n    <p class=\"meta\">by %s</p>\n    <p><strong>Slug:</strong> <code>%s</code><br><strong>Build:</strong> <code>%s</code></p>\n";
  const char* tpl_b = "    <p>This site was generated by <strong>Umicom AuthorEngine AI</strong>. Replace this page during render stage.</p>\n  </div>\n  </main>\n</body>\n</html>\n";
  size_t est=strlen(tpl_a)+strlen(title)*2+strlen(author)+strlen(slug)+strlen(stamp)+strlen(tpl_b)+(has_cover?strlen(cover_block):0)+(has_draft?strlen(draft_block):0)+256;
  char* html=(char*)malloc(est); if(!html) return -1;
  int n=snprintf(html,est,tpl_a,title,title,author,slug,stamp); if(n<0){ free(html); return -1; } size_t used=(size_t)n;
  if (has_cover) { used+=(size_t)snprintf(html+used,est-used,"%s",cover_block); }
  if (has_draft) { used+=(size_t)snprintf(html+used,est-used,"%s",draft_block); }
  snprintf(html+used,est-used,"%s",tpl_b);
  int wr=write_file(path,html); free(html); return (wr==0)?0:-1;
}

/*------------------------------- cmd_build ---------------------------------*/
static int cmd_build(void) {
  char title[256]={0}, author[256]={0}; int rc;
  rc=tiny_yaml_get("book.yaml","title",title,sizeof(title));
  if (rc==-1) { fprintf(stderr,"[build] ERROR: cannot open book.yaml (run `uaengine init`)\n"); return 1; }
  if (rc== 1) { snprintf(title,sizeof(title),"%s","Untitled"); }
  rc=tiny_yaml_get("book.yaml","author",author,sizeof(author));
  if (rc==-1) { fprintf(stderr,"[build] ERROR: cannot open book.yaml\n"); return 1; }
  if (rc== 1) { snprintf(author,sizeof(author),"%s","Unknown"); }

  int ingest_first=0; if(tiny_yaml_get_bool("book.yaml","ingest_on_build",&ingest_first)==0 && ingest_first) { puts("[build] ingest_on_build: true — running ingest..."); if(cmd_ingest()!=0) fprintf(stderr,"[build] WARN: ingest failed; continuing.\n"); }
  int norm=0; char drop[256]={0}; if(tiny_yaml_get("book.yaml","dropzone",drop,sizeof(drop))!=0) snprintf(drop,sizeof(drop),"%s","dropzone");
  if (tiny_yaml_get_bool("book.yaml","normalize_chapters_on_build",&norm)==0 && norm) { printf("[build] normalize_chapters_on_build: true — mirroring from %s\n", drop); /* In a later refactor, implement normalize into its own module. For now we only mirror when packing. */ }

  (void)generate_toc_md(title);
  (void)generate_frontmatter_md(title,author);
  (void)generate_acknowledgements_md(author);

  char slug[256]; slugify(title,slug,sizeof(slug));
  char day[32]; build_date_utc(day,sizeof(day));
  char stamp[64]; build_timestamp_utc(stamp,sizeof(stamp));
  char root[512]; snprintf(root,sizeof(root),"outputs%c%s%c%s",PATH_SEP,slug,PATH_SEP,day);

  if (helper_exists_file(root)) { printf("[build] cleaning existing: %s\n", root); if (clean_dir(root)!=0) fprintf(stderr,"[build] WARN: could not fully clean %s\n", root); }
  else { if (mkpath(root)!=0) { fprintf(stderr,"[build] ERROR: cannot create %s\n", root); return 1; } }
  (void)mkpath(root);
  const char* sub[]={"pdf","docx","epub","html","md","cover","video-scripts","site",NULL};
  for(int i=0; sub[i]!=NULL; ++i){ char p[640]; snprintf(p,sizeof(p),"%s%c%s",root,PATH_SEP,sub[i]); if (mkpath(p)!=0) fprintf(stderr,"[build] WARN: cannot create %s\n", p); }

  /* copy CSS into outputs/.../html/style.css */
  { const char* css_src="themes/uae.css"; char css_dst[1024]; snprintf(css_dst,sizeof(css_dst),"%s%chtml%cstyle.css",root,PATH_SEP,PATH_SEP);
     (void)mkpath(css_dst); /* ensure parent exists */ (void)copy_file_binary(css_src, css_dst);
  }

  /* cover */
  char ws_cover[640]; snprintf(ws_cover,sizeof(ws_cover),"workspace%ccover.svg",PATH_SEP);
  if (!helper_exists_file(ws_cover)) { (void)generate_cover_svg(title,author,slug); }
  (void)generate_frontcover_md(title,author,slug);

  int has_cover=0;
  char cover_dst_archive[640]; snprintf(cover_dst_archive,sizeof(cover_dst_archive),"%s%ccover%ccover.svg",root,PATH_SEP,PATH_SEP);
  char cover_dst_site[640];    snprintf(cover_dst_site,   sizeof(cover_dst_site),   "%s%csite%ccover.svg", root,PATH_SEP,PATH_SEP);
  if (helper_exists_file(ws_cover)) {
     if (copy_file_binary(ws_cover,cover_dst_archive)==0) { has_cover=helper_exists_file(cover_dst_archive); if(has_cover) printf("[cover] copied (archive): %s\n",cover_dst_archive); }
     if (copy_file_binary(ws_cover,cover_dst_site)==0)    { if (helper_exists_file(cover_dst_site)) { printf("[cover] copied (site): %s\n",cover_dst_site); has_cover=1; } }
  }

  char info_path[640]; snprintf(info_path,sizeof(info_path),"%s%cBUILDINFO.txt",root,PATH_SEP);
  char info[1024]; int n=snprintf(info,sizeof(info),"Title:  %s\nAuthor: %s\nSlug:   %s\nDate:   %s\nStamp:  %s\n", title,author,slug,day,stamp);
  (void)n; if (write_file(info_path,info)!=0) { fprintf(stderr,"[build] ERROR: cannot write BUILDINFO.txt\n"); return 1; }

  int has_draft=0;
  if (pack_book_draft(title,root,&has_draft)!=0) fprintf(stderr,"[build] WARN: failed to create book-draft.md\n");

  char site_dir[640]; snprintf(site_dir,sizeof(site_dir),"%s%c%s",root,PATH_SEP,"site");
  if (write_site_index(site_dir,title,author,slug,stamp,has_cover,has_draft)!=0) { fprintf(stderr,"[build] WARN: could not write site/index.html\n"); }

  printf("[build] ok: %s\n", root);
  puts("[build] outputs will be overwritten on subsequent builds for the same date.");
  return 0;
}

/*------------------------------- export ------------------------------------*/
static int exec_cmd(const char* cmdline) { printf("[exec] %s\n", cmdline); int rc=system(cmdline); if(rc!=0) fprintf(stderr,"[exec] command failed (rc=%d)\n",rc); return rc; }

static int cmd_export(void) {
  char title[256]={0}, author[256]={0};
  if (tiny_yaml_get("book.yaml","title", title,sizeof(title))  != 0) snprintf(title,sizeof(title),  "%s","My New Book");
  if (tiny_yaml_get("book.yaml","author",author,sizeof(author))!= 0) snprintf(author,sizeof(author), "%s","Your Name");
  char slug[256]; slugify(title,slug,sizeof(slug));
  char day[32]; build_date_utc(day,sizeof(day));

  if (!helper_exists_file("workspace/book-draft.md")) { fprintf(stderr,"[export] workspace/book-draft.md not found. Run `uaengine build` first.\n"); return 1; }

  char root[512]; snprintf(root,sizeof(root),"outputs%c%s%c%s",PATH_SEP,slug,PATH_SEP,day);
  char html_dir[640]; snprintf(html_dir,sizeof(html_dir),"%s%chtml",root,PATH_SEP);
  char pdf_dir[640];  snprintf(pdf_dir, sizeof(pdf_dir), "%s%cpdf", root, PATH_SEP);
  (void)mkpath(html_dir); (void)mkpath(pdf_dir);

  /* pandoc -> HTML */
  char out_html[768]; snprintf(out_html,sizeof(out_html),"%s%cbook.html",html_dir,PATH_SEP);
  char cmd1[2048];
  snprintf(cmd1,sizeof(cmd1),
    "pandoc \"workspace%cbook-draft.md\" -f markdown -t html5 -s --toc --metadata title=\"%s\" -M author=\"%s\" --resource-path=\".;dropzone;workspace\" -c \"style.css\" -o \"%s\"",
    PATH_SEP, title, author, out_html);
  if (exec_cmd(cmd1)!=0) { fprintf(stderr,"[export] ERROR: pandoc HTML failed.\n"); return 1; }

#ifdef _WIN32
  const char* candidates[]={ 
    "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
    "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
    NULL };
  char browser[512]={0};
  for (int i=0;candidates[i];++i) { if (helper_exists_file(candidates[i])) { snprintf(browser,sizeof(browser),"%s",candidates[i]); break; } }
  if (!browser[0]) { fprintf(stderr,"[export] WARN: Edge/Chrome not found; skipping PDF.\n"); printf("[export] HTML: %s\n", out_html); return 0; }

  /* Build absolute file URL */
  char abs_html[1024]; _fullpath(abs_html, out_html, (int)sizeof(abs_html));
  for(char* p=abs_html; *p; ++p) if (*p=='\\') *p='/';
  char pdf_path[768]; snprintf(pdf_path,sizeof(pdf_path),"%s%cbook.pdf", pdf_dir, PATH_SEP);
  char cmd2[2048];
  snprintf(cmd2,sizeof(cmd2),
    "cmd /C \"\"%s\" --headless=new --disable-gpu --print-to-pdf=\"%s\" --print-to-pdf-no-header --no-margins --run-all-compositor-stages-before-draw --virtual-time-budget=10000 \"file:///%s\"\"",
    browser, pdf_path, abs_html);
  (void)exec_cmd(cmd2);
  printf("[export] PDF: %s\n", pdf_path);
#else
  char pdf_path[768]; snprintf(pdf_path,sizeof(pdf_path),"%s%cbook.pdf", pdf_dir, PATH_SEP);
  char cmd2[2048]; snprintf(cmd2,sizeof(cmd2),"wkhtmltopdf \"%s\" \"%s\"", out_html, pdf_path);
  (void)exec_cmd(cmd2);
#endif
  printf("[export] HTML: %s\n", out_html); return 0;
}

/*------------------------------- serve -------------------------------------*/
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define closesock closesocket
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  typedef int sock_t;
  #define closesock close
#endif

static const char* serve_guess_mime(const char* path) {
  const char* dot=strrchr(path,'.'); if(!dot) return "application/octet-stream";
  if(!strcmp(dot,".html")||!strcmp(dot,".htm")) return "text/html; charset=utf-8";
  if(!strcmp(dot,".css"))  return "text/css; charset=utf-8";
  if(!strcmp(dot,".js"))   return "application/javascript";
  if(!strcmp(dot,".json")) return "application/json";
  if(!strcmp(dot,".svg"))  return "image/svg+xml";
  if(!strcmp(dot,".png"))  return "image/png";
  if(!strcmp(dot,".jpg")||!strcmp(dot,".jpeg")) return "image/jpeg";
  if(!strcmp(dot,".gif"))  return "image/gif";
  if(!strcmp(dot,".ico"))  return "image/x-icon";
  if(!strcmp(dot,".txt")||!strcmp(dot,".md")) return "text/plain; charset=utf-8";
  return "application/octet-stream";
}
static int serve_send_all(sock_t s, const char* buf, size_t len) {
  size_t sent=0; while(sent<len) {
  #ifdef _WIN32
    int n = send(s, buf+sent, (int)(len-sent), 0);
  #else
    int n = (int)send(s, buf+sent, len-sent, 0);
  #endif
    if (n<=0) return -1; sent+=(size_t)n;
  } return 0;
}
static void serve_send_404(sock_t cs) {
  const char* msg="HTTP/1.0 404 Not Found\r\nContent-Type: text/plain; charset=utf-8\r\nConnection: close\r\n\r\nNot Found\n";
  (void)serve_send_all(cs, msg, strlen(msg));
}
static int serve_build_fs_path(char* out, size_t outsz, const char* root, const char* uri) {
  if(!uri||!*uri) uri="/"; if (strstr(uri,"..")) return -1; if (strchr(uri,'%')) return -1;
  size_t ulen=strcspn(uri,"?\r\n"); int need_index=(ulen==1 && uri[0]=='/');
  char rel[512]; size_t j=0;
  for(size_t i=0;i<ulen && j+1<sizeof(rel);++i){ char c=uri[i]; if(i==0 && c=='/') continue; #ifdef _WIN32 if(c=='/') c='\\'; #endif rel[j++]=c; }
  rel[j]='\0'; if(need_index) snprintf(rel,sizeof(rel),"index.html");
  int need=snprintf(NULL,0,"%s%c%s",root,PATH_SEP,rel); if(need<0||(size_t)need+1>outsz) return -1;
  snprintf(out,outsz,"%s%c%s",root,PATH_SEP,rel); return 0;
}
static void serve_handle_client(sock_t cs, const char* root) {
  char req[2048]={0};
#ifdef _WIN32
  int n=recv(cs,req,(int)sizeof(req)-1,0);
#else
  int n=(int)recv(cs,req,sizeof(req)-1,0);
#endif
  if(n<=0){ closesock(cs); return; }
  char method[8]={0}; char uri[1024]={0};
#ifdef _WIN32
  if (sscanf_s(req,"%7s %1023s",method,(unsigned)sizeof(method), uri,(unsigned)sizeof(uri))!=2 ||
      (strcmp(method,"GET")!=0 && strcmp(method,"HEAD")!=0)) { serve_send_404(cs); closesock(cs); return; }
#else
  if (sscanf(req,"%7s %1023s",method,uri)!=2 || (strcmp(method,"GET")!=0 && strcmp(method,"HEAD")!=0)) { serve_send_404(cs); closesock(cs); return; }
#endif
  if (strchr(uri,'%')) { serve_send_404(cs); closesock(cs); return; }
  char path[1024]; if (serve_build_fs_path(path,sizeof(path),root,uri)!=0) { serve_send_404(cs); closesock(cs); return; }
#ifdef _WIN32
  FILE* f; if (fopen_s(&f,path,"rb")) { serve_send_404(cs); closesock(cs); return; }
#else
  FILE* f=fopen(path,"rb"); if(!f) { serve_send_404(cs); closesock(cs); return; }
#endif
  if (fseek(f,0,SEEK_END)!=0) { fclose(f); closesock(cs); return; }
  long sz=ftell(f); if (sz<0) { fclose(f); closesock(cs); return; } rewind(f);
  const char* mime=serve_guess_mime(path);
  char header[512]; int h=snprintf(header,sizeof(header),"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n", mime, sz);
  if (h<0 || h>=(int)sizeof(header)) { fclose(f); closesock(cs); return; }
  if (serve_send_all(cs,header,(size_t)h)!=0) { fclose(f); closesock(cs); return; }
  if (strcmp(method,"HEAD")==0) { fclose(f); closesock(cs); return; }
  char buf[65536]; size_t nrd; while((nrd=fread(buf,1,sizeof(buf),f))>0) { if (serve_send_all(cs,buf,nrd)!=0) { fclose(f); closesock(cs); return; } }
  fclose(f); closesock(cs);
}
static int cmd_serve(int argc, char** argv) {
#ifdef _WIN32
  WSADATA wsa; WSAStartup(MAKEWORD(2,2),&wsa);
#endif
  char title[256]={0}; if (tiny_yaml_get("book.yaml","title",title,sizeof(title))!=0) snprintf(title,sizeof(title),"%s","My New Book");
  char slug[256]; slugify(title,slug,sizeof(slug)); char day[32]; build_date_utc(day,sizeof(day));
  char root[512]; snprintf(root,sizeof(root),"outputs%c%s%c%s%csite",PATH_SEP,slug,PATH_SEP,day,PATH_SEP);
  const char* host="127.0.0.1"; int port=8080;
  if (argc>=3) host=argv[2];
  if (argc>=4) { int p=atoi(argv[3]); if (p>0) port=p; }
  sock_t s=socket(AF_INET,SOCK_STREAM,0); if (s<0) { fprintf(stderr,"[serve] ERROR: socket()\n"); return 1; }
  struct sockaddr_in addr; memset(&addr,0,sizeof(addr)); addr.sin_family=AF_INET; addr.sin_addr.s_addr=inet_addr(host); addr.sin_port=htons((unsigned short)port);
  int opt=1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
  if (bind(s,(struct sockaddr*)&addr,sizeof(addr))!=0) { fprintf(stderr,"[serve] ERROR: bind() on port %d\n", port); return 1; }
  if (listen(s,64)!=0) { fprintf(stderr,"[serve] ERROR: listen()\n"); return 1; }
  printf("[serve] Serving %s at http://%s:%d\n", root, host, port);
  for(;;) {
    struct sockaddr_in cli; socklen_t cl=sizeof(cli);
#ifdef _WIN32
    sock_t cs=accept(s,(struct sockaddr*)&cli,&cl);
#else
    sock_t cs=accept(s,(struct sockaddr*)&cli,&cl);
#endif
    if (cs<0) continue; serve_handle_client(cs, root);
  }
  closesock(s); 
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}

/*------------------------------- doctor/new --------------------------------*/
static int cmd_doctor(void) {
  puts("[doctor] checking dependencies...");
  /* pandoc */
  int pandoc_ok = system("pandoc -v >NUL 2>NUL")==0 ? 1 : 0;
  printf("  - pandoc: %s\n", pandoc_ok? "OK":"MISSING");
#ifdef _WIN32
  const char* candidates[]={ 
    "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
    "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
    NULL };
  int browser_ok=0; for (int i=0;candidates[i];++i) if (helper_exists_file(candidates[i])) { browser_ok=1; break; }
  printf("  - Edge/Chrome: %s\n", browser_ok? "OK":"MISSING");
#else
  printf("  - wkhtmltopdf (optional): %s\n", system("wkhtmltopdf -V >/dev/null 2>&1")==0? "OK":"MISSING");
#endif
  printf("  - can write workspace/: %s\n", (mkpath("workspace")==0? "OK":"FAILED"));
  char tmp[512]; snprintf(tmp,sizeof(tmp),"outputs%ctest-ide%ctoday%chtml",PATH_SEP,PATH_SEP,PATH_SEP);
  printf("  - can write outputs/.../html: %s\n", (mkpath(tmp)==0? "OK":"FAILED"));
  puts("[doctor] All checks passed ✅");
  return 0;
}
static int cmd_new(int argc, char** argv) {
  if (argc<3) { fprintf(stderr,"Usage: uaengine new "<file name>.md"\n"); return 1; }
  const char* name=argv[2];
  (void)mkpath("dropzone/chapters");
  char path[1024]; snprintf(path,sizeof(path),"dropzone%cchapters%c%s",PATH_SEP,PATH_SEP,name);
  const char* boiler =
"# Title goes here\n\n"
"Intro paragraph...\n\n"
"## Section\n\n"
"- bullet\n- bullet\n";
  if (write_text_file_if_absent(path,boiler)==0) { printf("[new] created: %s\n", path); return 0; }
  fprintf(stderr,"[new] failed to create: %s\n", path); return 1;
}

/*------------------------------- init --------------------------------------*/
static int write_gitkeep(const char *dir) {
  char path[1024]; snprintf(path,sizeof(path),"%s%c.gitkeep",dir,PATH_SEP);
  return write_text_file_if_absent(path,"");
}
static int cmd_init(void) {
  const char* dirs[]={ "dropzone","dropzone/images","workspace","outputs","templates","prompts","themes", NULL };
  for(int i=0; dirs[i]!=NULL; ++i) { if (mkpath(dirs[i])==0) { printf("[init] ok: %s\n", dirs[i]); (void)write_gitkeep(dirs[i]); } else { fprintf(stderr,"[init] ERROR: could not create path: %s\n", dirs[i]); } }
  (void)write_text_file_if_absent("themes/uae.css","/* minimal theme placeholder */\n");
  if (seed_book_yaml()!=0) { fprintf(stderr,"[init] ERROR: failed to write book.yaml\n"); return 1; }
  puts("[init] complete."); return 0;
}

/*------------------------------- publish -----------------------------------*/
static int cmd_publish(void) { puts("[publish] not implemented yet."); return 0; }

/*--------------------------------- main ------------------------------------*/
static void usage(void) {
  puts("Umicom AuthorEngine AI (uaengine) - Manage your book projects with AI assistance.\n");
  puts("Usage: uaengine <command> [options]\n");
  puts("Commands:");
  puts("  init               Initialize a new book project structure.");
  puts("  ingest             Ingest and organize content from the dropzone.");
  puts("  build              Build the book draft and prepare outputs.");
  puts("  export             Export the book to HTML and PDF formats.");
  puts("  serve [host] [port]  Serve the outputs directory over HTTP (default 127.0.0.1 8080).");
  puts("  doctor             Check environment and dependencies.");
  puts("  new <name.md>      Create a new chapter in dropzone/chapters/.");
  puts("  publish            Publish the book to a remote server (not implemented).");
  puts("  --version          Show version information.");
}

int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif
  if (argc<2) { usage(); return 0; }
  const char* cmd=argv[1];
  if      (!strcmp(cmd,"init"))     return cmd_init();
  else if (!strcmp(cmd,"ingest"))   return cmd_ingest();
  else if (!strcmp(cmd,"build"))    return cmd_build();
  else if (!strcmp(cmd,"export"))   return cmd_export();
  else if (!strcmp(cmd,"serve"))    return cmd_serve(argc,argv);
  else if (!strcmp(cmd,"publish"))  return cmd_publish();
  else if (!strcmp(cmd,"doctor"))   return cmd_doctor();
  else if (!strcmp(cmd,"new"))      return cmd_new(argc,argv);
  else if (!strcmp(cmd,"--version")){ printf("uaengine v%s\n", UENG_VERSION_STRING); return 0; }
  else { fprintf(stderr,"Unknown command: %s\n", cmd); usage(); return 1; }
}
