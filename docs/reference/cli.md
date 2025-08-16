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

Outputs timed listing of sourced files. `-l` prints full absolute paths.

## dirlist

```zsh
zpmod dirlist [-a] [-d|-f] array dir
```

List entries in `dir` into `array`.

## pathstat

```zsh
zpmod pathstat [-L] [-f fields] out_array in_array
```

Stat each path from `in_array` and write per-path records to `out_array`.

## readfile

```zsh
zpmod readfile [-m] [-d delim|-0] var file
```

Read file into scalar `var` or split into array using delimiter.
