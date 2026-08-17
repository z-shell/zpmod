# Use the zpreadarray Builtin

zpmod provides a `zpreadarray` builtin to read records into an indexed array.

Basic usage:

```zsh
print -r -- $'a\nb\nc' | zpreadarray A
print -r -- ${#A[@]}  # => 3
```

Common options:

| Option | Meaning                                  |
| ------ | ---------------------------------------- |
| -d X   | Record delimiter (default newline)       |
| -t     | Trim trailing delimiter from each record |
| -n N   | Read at most N records                   |
| -O I   | Start assigning at index I (1-based)     |
| -s N   | Skip first N records                     |
| -u FD  | Read from file descriptor FD             |
| -C cb  | Call callback after each quantum batch   |
| -c Q   | Batch size for -C (default 5000)         |

Examples:

```zsh
# Custom delimiter, keep delimiters
print -nr -- 'x,y,z,' | zpreadarray -d , B
print -r -- ${B[@]}   # 'x,' 'y,' 'z,' ''

# Custom delimiter, trim (-t)
print -nr -- 'x,y,z,' | zpreadarray -t -d , B
print -r -- ${B[@]}   # x y z ''

# Start at index 5
B=(1 2 3 4)
print -r -- $'a\nb' | zpreadarray -O 5 B
# B -> (1 2 3 4 a b)
```

See full details in [Reference > Builtins](../reference/builtins.md#zpreadarray).
