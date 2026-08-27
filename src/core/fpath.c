/* SPDX-License-Identifier: MIT */
/**
 * @file fpath.c
 * @brief FPATH indexing for autoload.
 */
/* Canonical module header ordering */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_fpath.h"
#include "zpmod_utils.h"
#include "zpmod_vendor_shims.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @brief Implements `zpmod fpath-index` with intelligent skip detection.
 *
 * Generates function-to-path mappings from FPATH directories with performance
 * optimization through skip detection that avoids rebuilding unchanged indexes.
 *
 * ## Index Format (v1)
 *
 * The generated index uses a structured format for reliable skip detection:
 * ```
 * # zpmod fpath-index v1
 * # dir 0 /first/fpath/dir 1234567890
 * # dir 1 /second/fpath/dir 1234567891
 * # dir 2 /third/fpath/dir -1
 * function_name /first/fpath/dir/function_name
 * other_func /second/fpath/dir/other_func
 * ```
 *
 * ## Skip Detection Algorithm
 *
 * The skip detection was redesigned to handle missing directories consistently,
 * fixing a critical issue where mtime mismatches caused unnecessary rebuilds.
 *
 * **Original Problem**: When a directory didn't exist during index generation,
 * we recorded `-1` as the mtime. However, if the directory appeared later or
 * stat() behaved differently during verification, we'd get a mismatch and
 * force an unnecessary rebuild.
 *
 * **Solution**: Unified mtime handling ensures consistent comparison:
 * - During generation: `stat()` failure → record `-1`
 * - During verification: `stat()` failure → use `-1` for comparison
 * - Both paths handle missing directories identically
 *
 * @param nam   Builtin name for error reporting
 * @param argv  Command arguments (--out, --rebuild, --preload)
 * @return 0 on success, 1 on failure
 *
 * @note The preload functionality populates shfunctab with function stubs
 *       for faster autoload resolution without writing index files.
 */
// NOLINTBEGIN(readability-function-cognitive-complexity)
int cmd_fpath_index(char *nam, char **argv) {
  const char *outfile = NULL;
  int rebuild = 0;
  int preload = 0;

  while (*argv && argv[0][0] == '-') {
    if (strcmp(argv[0], "--rebuild") == 0) {
      rebuild = 1;
      argv++;
      continue;
    }
    if (strcmp(argv[0], "--preload") == 0) {
      preload = 1;
      argv++;
      continue;
    }
    if (strcmp(argv[0], "--out") == 0) {
      if (!argv[1]) {
        zwarnnam(nam, "--out requires an argument");
        return 1;
      }
      outfile = argv[1];
      argv += 2;
      continue;
    }
    if (strcmp(argv[0], "--") == 0) {
      argv++;
      break;
    }
    break;
  }

  char **fpath = getaparam("fpath");
  if (!fpath) {
    return 0;
  }

  /*
   * INTELLIGENT SKIP DETECTION
   *
   * Attempts to detect when the existing index is still fresh and can be
   * reused, avoiding expensive directory scanning when FPATH directories
   * haven't changed.
   *
   * This optimization is crucial for startup performance in environments with
   * large FPATH configurations or slow filesystem access.
   */
  if (outfile && !rebuild) {
    FILE *rf = fopen(outfile, "r");
    if (rf) {
      char line[2048];
      int header_ok = 0;

      /* Verify index format version */
      if (fgets(line, sizeof(line), rf) &&
          strncmp(line, "# zpmod fpath-index v1", 22) == 0) {
        /*
         * HEADER VALIDATION STRATEGY
         *
         * We require the header to completely cover the current fpath
         * configuration. This ensures that any changes to fpath ordering,
         * additions, or removals will trigger a rebuild as expected by test
         * scenarios.
         *
         * Additionally, we validate directory mtimes to detect content changes
         * within existing directories (new functions added, etc.).
         */
        int idx = 0;
        header_ok = 1;

        while (fgets(line, sizeof(line), rf)) {
          if (strncmp(line, "# dir ", 6) != 0) {
            break; /* End of header section */
          }

          /* Parse header line: # dir <index> <path> <mtime> */
          char tag[8];
          int i_hdr = -1;
          char pathbuf[2048];
          long mt_recorded = 0;

          if (sscanf(line, "# %7s %d %2047s %ld", tag, &i_hdr, pathbuf,
                     &mt_recorded) != 4) {
            header_ok = 0;
            break;
          }

          /* Validate header entry format and sequencing */
          if (strcmp(tag, "dir") != 0 || i_hdr != idx) {
            header_ok = 0;
            break;
          }

          /* Ensure current fpath matches recorded path at same index */
          if (!fpath[idx] || strcmp(fpath[idx], pathbuf) != 0) {
            header_ok = 0;
            break;
          }

          /*
           * CONSISTENT MTIME HANDLING
           *
           * This is the critical fix for skip detection reliability.
           * We ensure that missing directories are handled identically
           * during both generation and verification phases.
           */
          struct stat st_now;
          long mt_current =
              -1; /* Default for missing/inaccessible directories */

          if (stat(fpath[idx], &st_now) == 0) {
            mt_current = (long)st_now.st_mtime;
          }
          /* If stat() fails, mt_current remains -1, matching generation
           * behavior */

          if (mt_current != mt_recorded) {
            header_ok = 0;
            break; /* Directory content changed or accessibility changed */
          }
          idx++;
        }

        /*
         * SKIP CONDITION VALIDATION
         *
         * We can safely skip rebuilding only if:
         * 1. All header entries were valid (header_ok = 1)
         * 2. We consumed exactly all current fpath entries (fpath[idx] == NULL)
         *
         * This ensures complete coverage and detects fpath changes like:
         * - New directories added to fpath
         * - Directories removed from fpath
         * - Directory reordering in fpath
         */
        if (header_ok && fpath[idx] == NULL) {
          fclose(rf);

          if (preload) {
            /*
             * PRELOAD MODE: Populate function table without file I/O
             *
             * When skip is detected but preload is requested, we scan
             * the directories to populate shfunctab with function stubs.
             * This provides the performance benefit of preloading without
             * the overhead of regenerating the index file.
             */
            for (int i = 0; i < idx; i++) {
              DIR *d = opendir(fpath[i]);
              if (!d) {
                continue;
              }

              struct dirent *de;
              while ((de = readdir(d))) {
                if (de->d_name[0] == '.' || de->d_name[0] == '_') {
                  continue; /* Skip hidden and private functions */
                }

                /* Only add if function not already loaded */
                if (shfunctab && !gethashnode2(shfunctab, de->d_name)) {
                  Shfunc sf = (Shfunc)zalloc(sizeof(*sf));
                  if (!sf) {
                    continue;
                  }

                  memset(sf, 0, sizeof(*sf));
                  sf->node.nam = dupstring(de->d_name);
                  sf->filename = ztrdup(fpath[i]);
                  addhashnode(shfunctab, sf->node.nam, &sf->node);
                }
              }
              closedir(d);
            }
          }
          return 0; /* Successfully skipped rebuild */
        }
      }
      fclose(rf);
    }
  }

  /* Build / preload */
  FILE *out_fp = stdout;
  if (outfile) {
    out_fp = zp_fopen_write_nofollow(outfile, 0644, 0);
    if (!out_fp) {
      zwarnnam(nam, "cannot open for writing: %s", outfile);
      return 1;
    }
    fprintf(out_fp, "# zpmod fpath-index v1\n");
  }

  /* Emit header with dir mtimes so we can skip next time */
  if (outfile) {
    struct stat st;
    for (int i = 0; fpath[i]; ++i) {
      long mt = -1;
      if (stat(fpath[i], &st) == 0) {
        mt = (long)st.st_mtime;
      }
      fprintf(out_fp, "# dir %d %s %ld\n", i, fpath[i], mt);
    }
  }

  for (int i = 0; fpath[i]; i++) {
    DIR *d = opendir(fpath[i]);
    if (!d) {
      continue;
    }
    struct dirent *de;
    while ((de = readdir(d))) {
      if (de->d_name[0] == '.' || de->d_name[0] == '_') {
        continue;
      }
      if (preload) {
#ifdef ZSH_OOT_MODULE
        /* When building out-of-tree we rely on zsh headers imported via
         * zpmod.mdh above; ensure symbols exist before using. */
#endif
        if (shfunctab && !gethashnode2(shfunctab, de->d_name)) {
          Shfunc sf = (Shfunc)zalloc(sizeof(*sf));
          if (!sf) {
            continue;
          }
          memset(sf, 0, sizeof(*sf));
          sf->node.nam = dupstring(de->d_name);
          sf->filename = ztrdup(fpath[i]);
          addhashnode(shfunctab, sf->node.nam, &sf->node);
        }
      }
      if (!preload) {
        fprintf(out_fp, "%s %s/%s\n", de->d_name, fpath[i], de->d_name);
      }
    }
    closedir(d);
  }

  if (outfile && out_fp != stdout) {
    fclose(out_fp);
  }
  return 0;
}
// NOLINTEND(readability-function-cognitive-complexity)
