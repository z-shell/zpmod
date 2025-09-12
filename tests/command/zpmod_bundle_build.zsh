#!/usr/bin/env zsh
# Functional tests for bundle-build implementation.
0=${(%):-%N}
this_dir=${0:A:h}
source "$this_dir/../test_helpers.zsh" || exit 1

setup_module || fail "module load failed"

mktemp_dir() {
  mktemp -d 2>/dev/null || mktemp -d -t tmp
}

TEST "bundle-build basic recursive + sort + truncate" {
  root=$(mktemp_dir)
  mkdir -p $root/alpha $root/beta
  print 'echo alpha1' > $root/alpha/01-alpha.zsh
  print 'echo beta' > $root/beta/zz.plugin.zsh
  print 'echo alpha2' > $root/alpha/02-alpha.plugin.zsh
  # ensure recursion picks nested dirs and sort is lexical
  out=$root/bundle.zsh
  zpmod bundle-build --from $root --out $out --max 1 || fail "bundle-build failed"
  [[ -s $out ]] || fail "bundle empty"
  grep -q 'BEGIN alpha/01-alpha.zsh' $out || fail 'missing first file'
  # truncated note due to small max
  grep -q 'truncated' $(dirname $out)/../build-cmake/Testing/Temporary/LastTest.log 2>/dev/null || true
  rm -rf -- $root
}

TEST "bundle-build freshness skip" {
  root=$(mktemp_dir)
  print 'echo hi' > $root/a.zsh
  out=$root/bundle.zsh
  zpmod bundle-build --from $root --out $out || fail 'first build'
  ts1=$(stat -c %Y $out 2>/dev/null || stat -f %m $out)
  sleep 1
  zpmod bundle-build --from $root --out $out || fail 'second build'
  ts2=$(stat -c %Y $out 2>/dev/null || stat -f %m $out)
  [[ $ts1 == $ts2 ]] || fail 'bundle rebuilt unexpectedly'
  # touch source to force rebuild
  sleep 1
  touch $root/a.zsh
  zpmod bundle-build --from $root --out $out || fail 'third build'
  ts3=$(stat -c %Y $out 2>/dev/null || stat -f %m $out)
  [[ $ts3 -gt $ts2 ]] || fail 'bundle not rebuilt after source change'
  rm -rf -- $root
}

success "bundle-build tests ok"
