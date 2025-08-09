# CLI / Subcommands

`zpmod` builtin accepts flags and subcommands.

Global flags:

- `-h` usage
- `-V` version

Subcommands:

## report-append

```
zpmod report-append <plugin-ID> <body>
```

Appends `<body>` to `ZI_REPORTS[plugin-ID]`. Non-zero status if plugin ID missing.

## source-study

```
zpmod source-study [ -l ]
```

Outputs timed listing of sourced files. `-l` prints full absolute paths.
