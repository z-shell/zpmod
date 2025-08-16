# Builtins

## custom_dot

Intercepts `.` and `source` to:

- Attempt use or creation of `.zwc` compiled form (via zcompile logic)
- Record timing meta for `zpmod source-study`

Errors follow standard zsh source semantics.

## zpmod

Primary entrypoint with subcommands:

```zsh
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

```zsh
readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] array
```

See: [How-to › Use readarray](../how-to/use-readarray.md)

## zppathstat

Batch file metadata lookup.

Synopsis:

```zsh
zppathstat [-L] [-f fields] path...
```

Options:

- `-L` follow symlinks (use `stat` instead of `lstat`)
- `-f` limit output to a comma-separated subset of fields

Fields:

- `type` (f=regular file, d=directory, l=symlink)
- `size` (bytes)
- `mode` (octal, 0000-07777)
- `mtime` (epoch seconds)
- `uid`, `gid` (numeric owners)
- `ino` (inode number)
- `nlink` (hard link count)

Output format: one line per path like `path=/tmp/x,type=f,size=12,mode=644,mtime=1700000000,uid=1000,gid=1000,ino=123,nlink=1,errno=0`.

`errno` is non-zero on error and `type/size/...` may be omitted depending on fields selected or errors.

## zpdirlist

Fast directory listing.

Synopsis:

```zsh
zpdirlist [-a] [-d|-f] array dir
```

Options:

- `-a` include dotfiles (default: skip `.`-prefixed)
- `-d` only directories
- `-f` only regular files

Writes names (not paths) into `array`.

## zpreadfile

Fast file reader into scalar or array.

Synopsis:

```zsh
zpreadfile [-m] [-d delim|-0] var file
```

Behavior:

- If `var` is a scalar, the entire contents are stored (no splitting)
- If `var` is an array, contents are split on the delimiter: `-d <delim>` or `-0` (NUL)
- `-m` may use `mmap` when available

Delimiter escapes accepted with `-d`: `\n`, `\r`, `\t`, `\0`.
