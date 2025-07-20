#!/bin/bash
# Script for checking TODO/FIXME comments

set -e

if grep -r "TODO\|FIXME\|XXX\|HACK" --include="*.c" --include="*.h" --include="*.sh" --include="*.zsh" Src/ Scripts/ 2>/dev/null; then
  echo "⚠️  Found TODO/FIXME comments. Consider creating issues for these."
  exit 0 # Changed from exit 1 to match original GitHub Actions behavior
fi

echo "✅ No TODO/FIXME comments found"
exit 0
