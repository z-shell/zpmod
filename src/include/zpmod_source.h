/* SPDX-License-Identifier: MIT */
#pragma once
#include "zpmod.mdh"
#include "zpmod.pro"

/* Public pieces of source-study facility */

typedef struct zp_sevent_node *SEventNode;

struct source_event {
  int id;
  long ts;
  char *dir_path;
  char *file_name;
  char *full_path;
  double duration;
  int load_error;
};

struct zp_sevent_node {
  struct hashnode node;
  struct source_event event;
};

/* Hash lifecycle */
void zp_free_sevent_node(HashNode hn);
char *zp_build_source_report(int no_paths, int *rep_size);

/* Overridden dot/source and helpers exported from core */
mod_export enum source_return custom_source(char *s);
Eprog custom_try_source_file(char *file);
