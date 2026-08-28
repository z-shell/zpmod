# Enable Debug Logging

Set the environment variable before loading the module:

```zsh
typeset -g ZI_MOD_DEBUG=1
module_path+=("/absolute/path/from/RESOLVED_MODULE_DIR")
zmodload zpmod
```

Use the exact `RESOLVED_MODULE_DIR` printed by the installer in place of the
placeholder.

You will see warnings when compilation is skipped or when a file cannot be accessed.

Disable by unsetting or setting to 0:

```zsh
unset ZI_MOD_DEBUG
# or
ZI_MOD_DEBUG=0
```
