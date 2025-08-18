#!/usr/bin/env zsh
# Validate source-study report formatting and options

TEST_NAME="core/source_study"

# shell options expected by harness
setopt EXTENDED_GLOB

# Load test helpers
. ${(q-)0:A:h}/../test_helpers.zsh || {
  print -ru2 -- "FATAL: couldn't source test_helpers.zsh"; return 1
}

# Ensure module loads and builtins are available
load_zpmod || die "Failed to load zpmod"
assert_builtin_exists zpmod

# Create temporary scripts to source
scratch_dir=$(mktemp -d ${TMPDIR:-/tmp}/zpmod-srcstudy-XXXXXX)
cleanup() { rm -rf -- $scratch_dir }
trap cleanup EXIT INT QUIT TERM

cat >! $scratch_dir/a.zsh <<'EOF'
#!/usr/bin/env zsh
emulate -LR zsh -o no_unset
# tiny work
: ${ZSH_VERSION+1}
EOF

cat >! $scratch_dir/b.zsh <<'EOF'
#!/usr/bin/env zsh
emulate -LR zsh
# small sleep to ensure non-zero duration
builtin sleep 0.01 2>/dev/null || true
EOF

# Source them via our custom dot
. $scratch_dir/a.zsh
source $scratch_dir/b.zsh

# Ask for a report (basenames only)
local rep
rep=$(zpmod source-study 2>&1)

# It should contain timing lines with emoji prefix and basenames
assert_match "⏱️" $rep
assert_match "a.zsh" $rep
assert_match "b.zsh" $rep
assert_not_match "/" $rep # no paths when -l not passed

# Now request full paths
local rep_full
rep_full=$(zpmod source-study -l 2>&1)
assert_match $scratch_dir $rep_full
assert_match "/a.zsh" $rep_full
assert_match "/b.zsh" $rep_full

# Quick sanity on formatting: ' ms    '
assert_match ' ms    ' $rep

print -r -- "source_study OK"
