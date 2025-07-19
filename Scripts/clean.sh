#!/bin/sh
#
# clean.sh - Clean up build artifacts and temporary files
#

# Stop on error
set -e

# Print commands as they are executed
if [ "${1}" = "--verbose" ] || [ "${1}" = "-v" ]; then
  set -x
fi

# Clean standard build artifacts
find . -type f \( -name "*.o" -o -name "*.so" -o -name "*.bundle" -o -name "*.a" -o -name "*.lo" -o -name "*.la" -o -name "*.dylib" \) -print0 | xargs -0 rm -f
find . -type f \( -name "*.log" -o -name "*.stamp" -o -name "*.cache" -o -name "*.out" -o -name "*.pyc" -o -name "*.pyo" \) -print0 | xargs -0 rm -f
find . -type f \( -name "*~" -o -name "*.swp" -o -name "*.swo" \) -print0 | xargs -0 rm -f

# Clean generated Makefiles
find . -name "Makefile" -print0 | xargs -0 rm -f

# Clean autoconf/automake files
rm -f config.log config.status config.h stamp-h

# Clean generated code files
find ./Src \( -name "*.mdh" -o -name "*.export" \) -print0 | xargs -0 rm -f
find ./Src \( -name "*.pro" -o -name "*.epro" \) -not -name ".indent.pro" -print0 | xargs -0 rm -f
find ./Src \( -name "*.mdhi" -o -name "*.mdhs" \) -print0 | xargs -0 rm -f

echo "Clean completed successfully"
