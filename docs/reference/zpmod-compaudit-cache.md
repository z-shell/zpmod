# `zpmod compaudit-cache`

Planned capabilities:

| Goal              | Detail                                                                |
| ----------------- | --------------------------------------------------------------------- |
| Cache parity      | Store secure/insecure verdicts matching `compaudit` logic             |
| Fast startup      | Skip redundant ownership/permission traversal when unchanged          |
| Safe invalidation | Invalidate when any directory's metadata (uid,gid,mode,ctime) changes |
| Introspection     | `--show` prints current cached verdict set                            |
| Rebuild           | `--rebuild` forces regeneration even if cache seems valid             |

Current implementation stores secure/insecure verdicts with metadata and validates cache before reuse.

## CLI

```sh
zpmod compaudit-cache [--rebuild] [--show] [--json]
```

## Roadmap / Progress

1. v2: Parent directory security propagation (ancestor flag stored)
2. v2: JSON output (`--json`) with per-directory `reasons` array
3. v2: Cache file permission validation (skips insecure mode and rebuilds)
4. v2: Incremental invalidation (in-place update of changed dirs + additions)
5. v2: Cached zwc writable flag to avoid repeat directory scans
6. v3: Sticky-bit + extended owner (root/EUID/exe-owner) parity & world/group write logic refinement
7. Future: symlink chain explicit lstat traversal & nuanced group filtering (RedHat per-user groups, Debian staff handling)
8. Future: watch/mtime batching for near real-time invalidation

## JSON Schema

```json
{
	"insecure": <int>,
	"secure": <int>,
	"dirs": [
		{
			"path": "<string>",
			"verdict": <0|1>,
			"parent_insecure": <0|1>,
			"reasons": [ "dir_perms"?, "ancestor_perms"?, "zwc_perms"? ]
		}
	]
}
```

Reason meanings:

- dir_perms: directory itself violates ownership/permissions policy
- ancestor_perms: an ancestor directory is insecure causing propagation
- zwc_perms: a compiled completion `.zwc` inside is writable by group/world

`zwc_perms` is cached in the on-disk entry and refreshed on rebuild or when a secure directory becomes insecure.

## Migration

The tool automatically migrates an existing `compaudit_v2.zcache` to `compaudit_v3.zcache` (triggering a rebuild) and cleans up the old
file. No manual action required.
