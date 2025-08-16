# Environment Variables

| Variable               | Purpose                                         | Values                                   |
| ---------------------- | ----------------------------------------------- | ---------------------------------------- |
| ZI_MOD_DEBUG           | Enable debug / verbose diagnostics              | `1` to enable, unset/0 to disable        |
| ZPMOD_ICONS            | Enable optional emoji/icons in certain outputs  | `1`/`true`/`on` or `0`/`false`/`off`     |
| ZPMOD_STAGE_MODULE_DIR | Test/staging helper path for loading the module | Absolute path to staged module directory |

Notes:

- Set before loading the module for earliest effect.
- `ZPMOD_STAGE_MODULE_DIR` is used by the CTest suite and local smoke tests to prepend to `module_path`.
