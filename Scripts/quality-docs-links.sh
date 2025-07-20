#!/bin/bash
# Script for checking documentation links

set -e

cd docs

# Find all markdown files with links
files_with_links=$(find . -name "*.md" -exec grep -l "\]\(" {} \; 2>/dev/null || true)

if [[ -z ${files_with_links} ]]; then
  echo "✅ No markdown links found to check"
  exit 0
fi

for file in ${files_with_links}; do
  # Extract links from each file
  links=$(grep -o '\]([^)]*\.md[^)]*)' "${file}" | sed 's/\](\([^)]*\))/\1/' 2>/dev/null || true)

  if [[ -n ${links} ]]; then
    echo "${links}" | while IFS= read -r link; do
      clean_link="${link%#*}"
      if [[ ${clean_link} == /* ]]; then
        target_file="$(pwd)${clean_link}"
      else
        target_file="$(dirname "${file}")/${clean_link}"
      fi
      if [[ ! -f ${target_file} ]]; then
        echo "❌ Broken link in ${file}: ${link} -> ${target_file}"
        exit 1
      fi
    done
  fi
done

echo "✅ Documentation links check passed"
exit 0
