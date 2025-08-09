git clone https://github.com/z-shell/zpmod.git
typeset -g ZI_MOD_DEBUG=1

# zpmod – Documentation Moved

This README is deprecated. Please use the structured documentation in the `docs/` directory (Divio pattern):

- Getting started tutorial: `docs/tutorials/first-use.md`
- Task guides: `docs/how-to/`
- Reference (builtins, env vars, install script): `docs/reference/`
- Explanations (architecture, compilation, profiling): `docs/explanation/`

Quick start:

```zsh
module_path+=("${HOME}/.zi/zmodules/zpmod/Src")

zpmod source-study
```

For installation options, see `docs/reference/install-script.md`.

Issues & feedback: https://github.com/z-shell/zpmod/issues

```sh

```
