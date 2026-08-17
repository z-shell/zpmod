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
zpmod source-study [ -l ]
```

Outputs timed listing of sourced files.

Notes:

- By default, only basenames are printed (e.g., `init.zsh`).
- Pass `-l` to print full absolute paths (e.g., `/home/user/.zshrc.d/init.zsh`).

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
