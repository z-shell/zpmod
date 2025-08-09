# Commenting guidelines for zpmod

This project uses Doxygen-compatible comments for API and module documentation.

- File headers: begin each source/header (`.c`, `.h`, `.mdh`, `.pro`) with a brief Doxygen block.
  - Example:
    /\*\*
    - \file src/zpmod.c
    - \brief zpmod zsh module implementation.
      \*/
- Functions (exported or complex): add a Doxygen block immediately above the declaration/definition.
  - Use `\brief`, `\param`, and `\return`. Prefer precise one-liners for `\brief`.
  - Example:
    /\*\*
    - \brief Load and enable zpmod builtins.
    - \param nam the builtin name (unused)
    - \param argv argument vector
    - \return status code (0 on success)
      \*/
- Internal helpers: brief comments as needed; avoid restating the obvious.
- Prefer comments that explain "why" over "what" when the code already says "what".
- Keep comments up to date; remove stale or misleading notes.

Doxygen usage:

- Docs are generated via `cmake --build build-cmake --target docs`.
- Output lives in `build-cmake/docs/html`.
