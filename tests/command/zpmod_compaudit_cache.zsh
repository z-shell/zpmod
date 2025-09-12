#!/usr/bin/env zsh
# Test initial compaudit-cache functionality: rebuild + show and metadata validation.

0=${(%):-%N}
this_dir=${0:A:h}
source "$this_dir/../test_helpers.zsh" || exit 1

setup_module || fail "module load failed"

# Create temporary secure + insecure directories and prepend to fpath
workdir=$(mktemp -d 2>/dev/null || mktemp -d -t tmp)
secure_dir="$workdir/secure"
insecure_dir="$workdir/insecure"
mkdir -p "$secure_dir" "$insecure_dir" || fail "mkdir failed"
# Make insecure world-writable directory
chmod 0777 "$insecure_dir" || fail "chmod failed"
# Prepend to fpath for this test only
local -a save_fpath
save_fpath=(${fpath[@]})
fpath=($secure_dir $insecure_dir $fpath)

TEST "compaudit-cache rebuild + show" {
  zpmod compaudit-cache --rebuild --show >$workdir/out.txt || fail "subcommand failed"
  grep -q "insecure" $workdir/out.txt || fail "missing insecure summary"
  grep -q "secure" $workdir/out.txt || fail "missing secure summary"
  grep -q "! $insecure_dir" $workdir/out.txt || fail "insecure dir not listed"
}

TEST "compaudit-cache version header v3" {
  local cache_file
  cache_file="$HOME/.cache/zpmod/compaudit_v3.zcache"
  zpmod compaudit-cache --rebuild --show >/dev/null || fail rebuild
  [[ -f $cache_file ]] || fail "cache file missing"
  head -n1 $cache_file | grep -q 'version:3' || fail "version header not v3"
}

TEST "compaudit-cache migration from v2" {
  local cachedir="$HOME/.cache/zpmod"
  mkdir -p "$cachedir" || fail mkdir
  rm -f "$cachedir/compaudit_v3.zcache" || true
  # Create fake v2 file with minimal header so migration triggers rebuild
  cat > "$cachedir/compaudit_v2.zcache" <<EOF
version:2
/tmp	0	755	0	0	0	0	0	0
EOF
  zpmod compaudit-cache --show >$workdir/migrate.txt || fail migrate
  [[ -f "$cachedir/compaudit_v3.zcache" ]] || fail "v3 not created after migration"
  head -n1 "$cachedir/compaudit_v3.zcache" | grep -q 'version:3' || fail "migrated header not v3"
  [[ ! -f "$cachedir/compaudit_v2.zcache" || -s "$cachedir/compaudit_v3.zcache" ]] || fail "legacy v2 not cleaned or v3 empty"
}

TEST "compaudit-cache json" {
  zpmod compaudit-cache --rebuild --json >$workdir/json.txt || fail "json failed"
  grep -q '"insecure"' $workdir/json.txt || fail 'json missing insecure key'
  grep -q '"dirs"' $workdir/json.txt || fail 'json missing dirs key'
  grep -q "$insecure_dir" $workdir/json.txt || fail 'json missing insecure path'
}

TEST "compaudit-cache incremental update detects new insecurity" {
  # First ensure cache exists with current state (only $insecure_dir flagged)
  zpmod compaudit-cache --rebuild --show >$workdir/first.txt || fail "initial rebuild failed"
  grep -q "! $insecure_dir" $workdir/first.txt || fail "baseline insecure missing"
  # Make previously secure dir insecure (world-writable) WITHOUT --rebuild
  chmod 0777 "$secure_dir" || fail "chmod insecure secure_dir failed"
  # Now invoke without --rebuild to exercise incremental path
  zpmod compaudit-cache --show >$workdir/second.txt || fail "incremental show failed"
  # Expect both dirs now listed as insecure (order not guaranteed)
  grep -q "! $insecure_dir" $workdir/second.txt || fail "original insecure missing after incremental"
  grep -q "! $secure_dir" $workdir/second.txt || fail "newly insecure dir not detected incrementally"
}

TEST "compaudit-cache ancestor reason json" {
  # Secure baseline
  mkdir -p $workdir/parent/child || fail "mkdir parent/child"
  chmod 0755 $workdir/parent $workdir/parent/child || fail "chmod baseline"
  fpath=($workdir/parent/child $fpath)
  zpmod compaudit-cache --rebuild --json >$workdir/a1.json || fail "baseline json rebuild"
  ! grep -q '"ancestor_perms"' $workdir/a1.json || fail "unexpected ancestor reason baseline"
  # Make parent world-writable then show (incremental path)
  chmod 0777 $workdir/parent || fail "chmod parent insecure"
  zpmod compaudit-cache --json >$workdir/a2.json || fail "json after parent change"
  grep -q '"ancestor_perms"' $workdir/a2.json || fail "missing ancestor_perms reason"
}

TEST "compaudit-cache zwc perms reason json" {
  mkdir -p $workdir/zwcdir || fail mkdir
  : > $workdir/zwcdir/test.zwc || fail touch
  chmod 0666 $workdir/zwcdir/test.zwc || fail chmod
  fpath=($workdir/zwcdir $fpath)
  zpmod compaudit-cache --rebuild --json >$workdir/z1.json || fail json
  grep -q '"zwc_perms"' $workdir/z1.json || fail "missing zwc_perms reason"
}

# Restore fpath
fpath=(${save_fpath[@]})
rm -rf -- "$workdir"

success "compaudit-cache tests ok"
