/* SPDX-License-Identifier: MIT */
#pragma once
#include "zpmod.mdh"
#include "zpmod.pro"

/**
 * @file zpmod_source.h
 * @brief Public interfaces for source-study and source overrides.
 *
 * This module instruments sourcing operations to record durations and produce
 * reports, and provides an override for `.`/`source` builtins to integrate the
 * behavior.
 */

typedef struct zp_sevent_node *SEventNode;

/**
 * @brief Recorded event for a sourced script.
 */
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

/** Hash lifecycle */
void zp_free_sevent_node(HashNode hn);

/** Overridden dot/source and helpers exported from core */
mod_export enum source_return custom_source(char *s);
Eprog custom_try_source_file(char *file);

/** Print all events (count 0), or the newest count events, without clearing them. */
int zp_source_study_core(const char *nam, int report_count, int full_paths);
