# CLI / Subcommands

`zpmod` builtin accepts flags and subcommands.

Global flags:

- `-h` usage
- `-V` version

Subcommands:

## report-append

```zsh
zpmod report-append <plugin-ID> <body>
```

Appends `<body>` to `ZI_REPORTS[plugin-ID]`. Non-zero status if plugin ID missing.

## source-study

```zsh
zpmod source-study [-l] [--json] [count]
```

Outputs timed listing of sourced files.

Notes:

- By default, only basenames are printed (e.g., `init.zsh`).
- Pass `-l` to print full absolute paths (e.g., `/home/user/.zshrc.d/init.zsh`); it does not clear history.
- The default (or count `0`) prints every recorded event. A positive decimal count prints the newest events in source-completion order.
- Empty history prints `No source events recorded.` and succeeds. Unavailable profiling or invalid arguments fail.
- Human-readable times are rounded to whole milliseconds and include nested sourcing. They are diagnostic observations, not a CI performance
  gate.

### JSON schema 1

`zpmod source-study --json [count]` emits one JSON object followed by a newline. Reporting never clears history. `-l` is accepted with
`--json`, but JSON always identifies absolute paths.

| Field                | Meaning                                                                                                            |
| -------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `schema_version`     | Integer `1`; consumers must reject unsupported versions.                                                           |
| `status`             | `empty`, `complete`, `incomplete`, or `unavailable`.                                                               |
| `clock`, `monotonic` | `CLOCK_MONOTONIC` (or macOS `CLOCK_MONOTONIC_RAW`) and `true`; otherwise `gettimeofday` and `false`.               |
| `unit`               | `nanoseconds`; integer storage preserves sub-millisecond durations, without promising nanosecond clock resolution. |
| `inclusive`          | Always `true`: a parent's duration includes nested sources. Do not sum overlapping events as elapsed wall time.    |
| `order`              | `completion`: children finish before their parents.                                                                |
| `execution_mode`     | `instrumented-auto-compile`: loading zpmod can create or use compiled files and adds profiling overhead.           |
| `events`             | Array of records described below; `[]` for empty or unavailable profiling.                                         |

Each event has an increasing integer `id`, nesting `depth` (top-level source is `1`), `source_status`, `exit_status`, `path_hex`, and
`duration_ns`. A positive count selects the newest events while retaining their original IDs and completion order. Repeated sources remain
separate records.

`source_status` is `0` for a successfully evaluated source, `1` for an open failure, or `2` for a source evaluation error. `exit_status` is
the resulting source command status, including a nonzero `return` from an otherwise successfully evaluated file. Failed attempts reaching
the source evaluator are recorded; PATH candidates rejected before evaluation are not events.

`path_hex` is lowercase hexadecimal encoding of the raw path bytes. The lexical absolute path is captured before evaluation, so a sourced
script changing directories cannot alter its identity. Symlinks and `..` components are not canonicalized. Hex preserves quotes, newlines,
backslashes and non-UTF-8 POSIX names without invalid JSON. Python consumers can decode it with `bytes.fromhex(event["path_hex"])`. Paths
may identify private directories; consider that before publishing profiling artifacts.

`duration_ns` is a nonnegative integer, or `null` if either clock read fails or an elapsed interval is invalid. Clock failures, backwards
intervals, overflow or lost records make the report `incomplete` and the command fail. A platform without a supported monotonic clock uses
the explicitly marked wall-clock fallback; runtime clock failures do not silently switch clocks. Consumers requiring monotonic measurements
must check `monotonic`.

`complete` describes capture completeness, not workload success: inspect each event's `source_status` and `exit_status`. Empty and complete
reports return zero; incomplete or unavailable profiling, invalid arguments and output errors return nonzero. Output errors can leave a
truncated document. The JSON format is diagnostic evidence, not the shared benchmark-comparison schema or an automatic CI performance gate.

## dir-list

```zsh
zpmod dir-list [-a] [-d|-f] array dir
```

List entries in `dir` into `array`.

## path-stat

```zsh
zpmod path-stat [-L] [-f fields] out_array in_array
```

Stat each path from `in_array` and write per-path records to `out_array`.

## path-warmup

```zsh
zpmod path-warmup [-q] [--prune-missing]
```

Scan `$PATH` directories and touch executable entries to warm filesystem caches.

Notes:

- `-q` runs quietly (recommended for startup hooks).
- `--prune-missing` is reserved for future behavior; currently a no-op.

## read-file

```zsh
zpmod read-file [-m] [-d delim|-0] var file
```

Read file into scalar `var` or split into array using delimiter.

Notes:

- If `var` is a scalar, the entire contents are stored.
- If `var` is an array, records are split on the delimiter provided with `-d <delim>` or `-0` (NUL).
- Escapes accepted with `-d`: `\n`, `\r`, `\t`, `\0`.
- When splitting on `\r`, a `\r\n` sequence (CRLF) is treated as a single separator.
- Trailing delimiters do not produce a trailing empty element (e.g., `a\nb\n` with `-d "\n"` yields `("a" "b")`).
