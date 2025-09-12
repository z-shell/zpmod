#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_path_warmup"
load_zpmod

# Should succeed quietly and not error
zpmod path-warmup -q || exit 1

# Allow running twice (idempotent)
zpmod path-warmup -q || exit 1

# --prune-missing flag should be accepted (behavior is no-op in prototype)
zpmod path-warmup -q --prune-missing || exit 1

test_status "PASS" "$TEST_NAME"
exit 0
