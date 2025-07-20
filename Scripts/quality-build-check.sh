#!/bin/bash
# Script for checking build system consistency

set -e

cd Src
for c_file in *.c zi/*.c; do
  if [[ -f ${c_file} ]]; then
    syms_file="${c_file%.c}.syms"
    if [[ ! -f ${syms_file} ]]; then
      echo "❌ Missing symbols file ${syms_file} for ${c_file}"
      exit 1
    fi
  fi
done

echo "✅ Build system consistency check passed"
exit 0
