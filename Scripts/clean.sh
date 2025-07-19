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
find . -type f -name "*.o" -o -name "*.so" -o -name "*.bundle" -o -name "*.a" -o -name "*.lo" -o -name "*.la" -o -name "*.dylib" | xargs rm -f
find . -type f -name "*.log" -o -name "*.stamp" -o -name "*.cache" -o -name "*.out" -o -name "*.pyc" -o -name "*.pyo" | xargs rm -f
find . -type f -name "*~" -o -name "*.swp" -o -name "*.swo" | xargs rm -f

# Clean generated Makefiles
find . -name "Makefile" | xargs rm -f

# Clean autoconf/automake files
rm -f config.log config.status config.h stamp-h

# Clean generated code files
find ./Src -name "*.mdh" -o -name "*.export" | xargs rm -f
find ./Src -name "*.pro" -o -name "*.epro" | grep -v ".indent.pro" | xargs rm -f
find ./Src -name "*.mdhi" -o -name "*.mdhs" | xargs rm -f

echo "Clean completed successfully"
