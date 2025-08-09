# Builtins

## custom_dot

Intercepts `.` and `source` to:

- Attempt use or creation of `.zwc` compiled form (via zcompile logic)
- Record timing meta for `zpmod source-study`

Errors follow standard zsh source semantics.

## zpmod

Primary entrypoint with subcommands:

```
zpmod [ -h | -V ]
zpmod report-append <plugin-ID> <body>
zpmod source-study [ -l ]
```

Flags:

- `-h` show usage
- `-V` show version

Subcommands:

- `report-append` – append body text to `ZI_REPORTS[plugin-ID]`
- `source-study` – print profile table (use -l for full paths)

Return codes:

- 0 success
- 1 usage / parameter errors

## readarray

Bash-like record reader into indexed arrays.

Synopsis:

```
readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] array
```

See: [How-to › Use readarray](../how-to/use-readarray.md)
