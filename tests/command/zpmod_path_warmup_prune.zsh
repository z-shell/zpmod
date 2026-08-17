#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_path_warmup_prune"
load_zpmod

# Prepare a fake path with a missing component
local td
td=$(mktemp -d)
local missing="$td/does-not-exist"
mkdir -p "$td/bin"
local -a save_path=( $path )
path=( "$td/bin" "$missing" $path )

# Dry run should not remove missing component
zpmod path-warmup -q --prune-missing --dry-run
[[ " ${path[*]} " == *" $missing "* ]]

# Actual prune should remove it
zpmod path-warmup -q --prune-missing
[[ " ${path[*]} " != *" $missing "* ]]

# Restore path
path=( $save_path )

test_status "PASS" "$TEST_NAME"
exit 0
