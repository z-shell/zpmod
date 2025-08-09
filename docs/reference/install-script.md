# Installation Script Options

`scripts/install.sh` supports:

| Option                      | Description                                                |
| --------------------------- | ---------------------------------------------------------- |
| --target DIR / --target=DIR | Install to specific directory                              |
| --clean                     | Run `make distclean` instead of `make clean`               |
| --quiet, -q                 | Suppress non-essential output                              |
| --verbose, -v               | Verbose build messages                                     |
| --no-git                    | Skip git clone/pull                                        |
| --force, -f                 | Force rebuild even if Makefile exists                      |
| --build-only                | Build only; do not modify shell config                     |
| --cflags=...                | Custom CFLAGS (default: `-g -Wall -Wextra -O3`)            |
| --branch=NAME               | Use specific git branch (default: current / main fallback) |
| --zsh-path=PATH             | Use specific zsh executable                                |
| --jobs=N / -jN              | Parallel make jobs                                         |
| --prefix=DIR                | Installation prefix (implies default target under prefix)  |
| --no-install                | Build but skip install step                                |
| --help, -h                  | Show help                                                  |
