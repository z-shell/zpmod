#!/usr/bin/env zsh
# This script syncs the needed files from a Zsh source tree to the zpmod module.
# It should be run when updating to a new version of Zsh.

emulate -L zsh
setopt extendedglob warncreateglobal noshortloops

# Check for help request or missing argument
if [[ -z "$1" || "$1" = "-h" || "$1" = "--help" ]]; then
    print "Usage: $0 <path-to-zsh-source>"
    print "  <path-to-zsh-source>: Path to Zsh source tree"
    print "\nThis script synchronizes zpmod with the provided Zsh source."
    print "WARNING: Will invoke git clean -dxf to ensure a clean workspace."
    exit 0
fi

# Check if source path exists
if [[ ! -d "$1" ]]; then
    print "Error: Path to Zsh source doesn't exist: $1"
    exit 1
fi

# Confirm before cleaning
print "WARNING: Will invoke git clean -dxf, which removes all untracked files."
print "Press Ctrl+C to abort or Enter to continue..."
read -q "?Are you sure you want to continue? [y/N] " || { print "\nAborted."; exit 0 }
print

# Clean the repository
git clean -dxf

local from="${1:A}"  # Get absolute path
print "Syncing from: $from"

autoload -Uz colors
colors

integer count=0

for i in configure.ac Src/*.c Src/*.h; do
    if [[ -f "$from/$i" ]]; then
        cp -vf "$from/$i" "$i" && (( ++ count )) || print "${fg_bold[red]}Copy error for: $i${reset_color}"
    else
        print "${fg[red]}$i Doesn't exist${reset_color}"
    fi
done

echo "${fg[green]}Copied ${fg[yellow]}$count${fg[green]} files${reset_color}"

patch -p2 -i ./patch_cfgac.diff
