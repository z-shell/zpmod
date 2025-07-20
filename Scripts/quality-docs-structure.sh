#!/bin/bash
# Script for checking documentation structure

set -e

required_dirs=("docs/tutorials" "docs/how-to" "docs/reference" "docs/explanation")
for dir in "${required_dirs[@]}"; do
  if [[ ! -d ${dir} ]]; then
    echo "❌ Missing required documentation directory ${dir}"
    exit 1
  fi
  if [[ ! -f "${dir}/README.md" ]]; then
    echo "❌ Missing README.md in ${dir}"
    exit 1
  fi
done

echo "✅ Divio documentation structure is valid"
exit 0
