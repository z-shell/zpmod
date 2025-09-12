/* SPDX-License-Identifier: MIT */
/**
 * @file rehash_diff.c
 * @brief Incremental PATH diff snapshot (initial slice).
 *
 * Goal: avoid full command rehash when PATH unchanged. This slice:
 *  - Persists a snapshot file with directory list + per-dir mtime+inode hash
 *  - On invocation, loads snapshot and computes diff: added, removed, metadata-changed
 *  - Prints summary and lists changed dirs
 *  - Future slices: selectively invoke internal hash invalidation for changed dirs only
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_vendor_shims.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include "zpmod_rehash.h"
#include "zpmod_emoji.h"

#define ZP_RH_SUBDIR "zpmod"
#define ZP_RH_FILE   "rehash_path_v1.snapshot"

static char *zp_rh_base_cache_dir(void) {
  const char *xdg = getsparam("XDG_CACHE_HOME");
  const char *home = getsparam("HOME");
  const char *base = (xdg && *xdg) ? xdg : NULL;
  if (!base && home && *home) {
    size_t len = strlen(home) + sizeof("/.cache");
    char *p = (char *)zalloc(len); if (!p) { return NULL;
}strcpy(p, home); strcat(p, "/.cache"); return p;
  }
  if (base) { return ztrdup(base);
}
  return NULL;
}

static char *zp_rh_dir(char *nam) {
  char *base = zp_rh_base_cache_dir(); if (!base) { zwarnnam(nam, "%srehash-diff: cannot resolve cache directory", zp_icon("❌ ")); return NULL; }
  size_t len = strlen(base) + 1 + strlen(ZP_RH_SUBDIR) + 1;
  char *full = (char *)zalloc(len); if (!full) { zsfree(base); return NULL; }
  sprintf(full, "%s/%s", base, ZP_RH_SUBDIR); zsfree(base);
  struct stat st; if (stat(full,&st)!=0) { if (mkdir(full, 0700)!=0) { zwarnnam(nam, "%srehash-diff: cannot create %s: %s", zp_icon("❌ "), full, strerror(errno)); zsfree(full); return NULL; } }
  return full;
}

static char *zp_rh_file(char *nam) {
  char *dir = zp_rh_dir(nam); if (!dir) { return NULL;
}
  size_t len = strlen(dir)+1+strlen(ZP_RH_FILE)+1; char *p=(char*)zalloc(len); if(!p){ zsfree(dir); return NULL; }
  sprintf(p, "%s/%s", dir, ZP_RH_FILE); zsfree(dir); return p;
}

struct rh_entry { char *path; unsigned long ino; time_t mtime; int present_now; };
struct rh_vec { struct rh_entry *items; size_t size; size_t cap; };
static void rh_vec_init(struct rh_vec *v){ v->items=NULL; v->size=0; v->cap=0; }
static int rh_vec_push(struct rh_vec *v, struct rh_entry *e){ if(v->size==v->cap){ size_t nc=v->cap? v->cap*2:16; void *nb=zrealloc(v->items, nc*sizeof(struct rh_entry)); if(!nb) { return 1;
}v->items=(struct rh_entry*)nb; v->cap=nc;} v->items[v->size++]=*e; return 0; }
static void rh_vec_free(struct rh_vec *v){ if(!v) { return;
}for(size_t i=0;i<v->size;i++){ if(v->items[i].path) { zsfree(v->items[i].path);
}} if(v->items) { zfree(v->items, v->cap*sizeof(struct rh_entry));
}v->items=NULL; v->size=v->cap=0; }

/* Load snapshot; returns 0 success, 1 error */
static int rh_load(const char *nam, const char *file, struct rh_vec *out){ // NOLINT(bugprone-easily-swappable-parameters)
  FILE *fp=fopen(file,"r"); if(!fp) { return 1;
}char line[4096]; if(!fgets(line,sizeof(line),fp)){ fclose(fp); return 1;} if(strncmp(line,"version:1",9)!=0){ fclose(fp); return 1;} while(fgets(line,sizeof(line),fp)){ if(line[0]=='\n'||line[0]=='#') { continue;
}char *nl=strchr(line,'\n'); if(nl) { *nl='\0';
}char *save=NULL; char *p=strtok_r(line,"\t", &save); if(!p) { continue;
}char *ino_s=strtok_r(NULL,"\t",&save); char *mt_s=strtok_r(NULL,"\t",&save); if(!ino_s||!mt_s) { continue;
}unsigned long ino=strtoul(ino_s,NULL,10); long mt=strtol(mt_s,NULL,10); size_t l=strlen(p)+1; char *dup=(char*)zalloc(l); if(!dup) { break;
}memcpy(dup,p,l); struct rh_entry e; e.path=dup; e.ino=ino; e.mtime=(time_t)mt; e.present_now=0; if(rh_vec_push(out,&e)){ zsfree(dup); break; } } fclose(fp); (void)nam; return 0; }

static int rh_write(char *nam, const char *file, struct rh_vec *paths){ FILE *fp=fopen(file,"w"); if(!fp){ zwarnnam(nam, "%srehash-diff: cannot write %s: %s", zp_icon("❌ "), file, strerror(errno)); return 1;} fprintf(fp,"version:1\n"); for(size_t i=0;i<paths->size;i++){ struct stat st; if(stat(paths->items[i].path,&st)!=0) { continue;
}fprintf(fp, "%s\t%lu\t%ld\n", paths->items[i].path, (unsigned long)st.st_ino, (long)st.st_mtime); } fclose(fp); return 0; }

static void rh_collect_current(struct rh_vec *out){ char **arr=getaparam("path"); if(!arr) { return; }
  for(char **path_it=arr; *path_it; ++path_it){ if(!**path_it) { continue; }
    size_t len=strlen(*path_it)+1; char *dup=(char*)zalloc(len); if(!dup) { break; }
    memcpy(dup,*path_it,len); struct rh_entry e; e.path=dup; e.ino=0; e.mtime=0; e.present_now=1; if(rh_vec_push(out,&e)){ zsfree(dup); break; }
  }
}

int zp_rehash_diff_core(char *nam) {
  int ret=0; char *file = zp_rh_file(nam); if(!file) { return 1;
}
  struct rh_vec prev; rh_vec_init(&prev); int have_prev = (rh_load(nam, file, &prev)==0);
  struct rh_vec now; rh_vec_init(&now); rh_collect_current(&now);
  /* Map prev entries by path via linear scan (PATH lengths are small) */
  int added=0;
  int removed=0;
  int changed=0;
  int unchanged=0;
  for(size_t i=0;i<now.size;i++) {
    struct rh_entry *cur=&now.items[i];
    struct stat st; if(stat(cur->path,&st)!=0) { continue; }
    cur->ino=st.st_ino; cur->mtime=st.st_mtime;
    int found=0; if(have_prev){ for(size_t j=0;j<prev.size;j++){ if(strcmp(prev.items[j].path, cur->path)==0){ found=1; prev.items[j].present_now=1; if(prev.items[j].ino!=cur->ino || prev.items[j].mtime!=cur->mtime){ changed++; } else { unchanged++; } break; } } }
    if(!found){ added++; }
  }
  if(have_prev){ for(size_t j=0;j<prev.size;j++){ if(!prev.items[j].present_now) { removed++;
}}}

  /* Write new snapshot (always, keeps last state) */
  if(rh_write(nam, file, &now)!=0) { ret=1; }
  fprintf(stdout, "%srehash-diff: added=%d removed=%d changed=%d unchanged=%d\n", zp_icon("🔄 "), added, removed, changed, unchanged);
  if (added)   { fprintf(stdout, "  + dirs: "); for(size_t i=0, printed=0;i<now.size;i++){ int f=0; if(!have_prev){ f=1; } else { for(size_t j=0;j<prev.size;j++){ if(strcmp(prev.items[j].path, now.items[i].path)==0){ f=1; break; } } f = !f; } if(f){ if(printed++) { fputc(' ', stdout);
}fputs(now.items[i].path, stdout);} } fputc('\n', stdout); }
  if (removed) { fprintf(stdout, "  - dirs: "); for(size_t j=0, printed=0;j<prev.size;j++){ if(!prev.items[j].present_now){ if(printed++) { fputc(' ', stdout);
}fputs(prev.items[j].path, stdout);} } fputc('\n', stdout); }
  if (changed) { fprintf(stdout, "  * dirs: "); for(size_t i=0, printed=0;i<now.size;i++){ for(size_t j=0;j<prev.size;j++){ if(strcmp(prev.items[j].path, now.items[i].path)==0 && (prev.items[j].ino!=now.items[i].ino || prev.items[j].mtime!=now.items[i].mtime)){ if(printed++) { fputc(' ', stdout);
}fputs(now.items[i].path, stdout); break; } } } fputc('\n', stdout); }
  fflush(stdout);
  rh_vec_free(&prev); rh_vec_free(&now); zsfree(file);
  return ret;
}
