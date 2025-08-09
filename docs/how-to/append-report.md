# Append to a Plugin Report

`zpmod report-append <plugin-ID> <body>` extends the string stored in `ZI_REPORTS[plugin-ID]`.

Example:

```zsh
typeset -gA ZI_REPORTS
ZI_REPORTS["z-shell/example"]='seed'

zpmod report-append z-shell/example '+more'
print -r -- ${ZI_REPORTS["z-shell/example"]}  # seed+more
```

Return status is non-zero if the plugin key does not exist or parameters are missing.
