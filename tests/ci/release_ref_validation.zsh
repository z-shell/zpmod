#!/usr/bin/env zsh
# Deterministic tests for the release-ref validator.
emulate -L zsh
set -euo pipefail

validator="${0:A:h:h:h}/scripts/validate-release-ref.zsh"
workdir=$(mktemp -d 2>/dev/null || mktemp -d -t zpmod-release-ref)
repo="$workdir/repo"
mkdir -p "$repo"
trap 'rm -rf -- "$workdir"' EXIT

git -C "$repo" init -q -b main
git -C "$repo" config user.name 'zpmod tests'
git -C "$repo" config user.email 'zpmod-tests@example.invalid'
print -r -- base >"$repo/file"
git -C "$repo" add file
git -C "$repo" commit -q -m 'test: base'
base_commit=$(git -C "$repo" rev-parse HEAD)
git -C "$repo" tag -a v1.2.3 -m 'v1.2.3'

run_validator() {
  (cd "$repo" && "$validator" "$@")
}

expect_failure() {
  local description=$1
  shift
  if run_validator "$@" >/dev/null 2>&1; then
    print -ru2 -- "validator unexpectedly accepted: $description"
    exit 1
  fi
}

actual=$(run_validator v1.2.3 refs/heads/main)
[[ $actual == "$base_commit" ]] || {
  print -ru2 -- "validator returned $actual, expected $base_commit"
  exit 1
}

# Remote mode must refresh a stale local tag before returning a commit.
remote="$workdir/remote.git"
consumer="$workdir/consumer"
git clone -q --bare "$repo" "$remote"
git clone -q "$remote" "$consumer"
remote_actual=$(cd "$consumer" &&
  "$validator" v1.2.3 refs/remotes/origin/main origin)
[[ $remote_actual == "$base_commit" ]] || {
  print -ru2 -- "remote validator returned $remote_actual, expected $base_commit"
  exit 1
}
if (cd "$consumer" &&
  "$validator" v1.2.3 origin/main origin) >/dev/null 2>&1; then
  print -ru2 -- 'remote validator accepted an ambiguous shorthand main ref'
  exit 1
fi
[[ ! -e "$consumer/.git/refs/heads/origin/main" ]] || {
  print -ru2 -- 'remote validator created an ambiguous local origin/main branch'
  exit 1
}
git -C "$repo" commit -q --allow-empty -m 'test: move release candidate'
moved_commit=$(git -C "$repo" rev-parse HEAD)
git -C "$repo" tag -f -a v1.2.3 -m 'moved v1.2.3' >/dev/null
git -C "$repo" push -q --force "$remote" main refs/tags/v1.2.3
remote_actual=$(cd "$consumer" &&
  "$validator" v1.2.3 refs/remotes/origin/main origin)
[[ $remote_actual == "$moved_commit" ]] || {
  print -ru2 -- "remote validator did not refresh moved tag"
  exit 1
}

# A reviewed ancestor is insufficient: publication must use the current main
# tip so the release workflow never executes a dynamically selected checkout.
git -C "$repo" tag -a v1.3.0 "$base_commit" -m 'v1.3.0 on old main'
expect_failure 'tag on an old main ancestor' v1.3.0 refs/heads/main

# Lightweight tags are not release boundaries.
git -C "$repo" tag v1.2.4
expect_failure 'lightweight tag' v1.2.4 refs/heads/main

# Pre-release and malformed refs are outside the final vX.Y.Z boundary.
git -C "$repo" tag -a v1.2.5-rc1 -m 'v1.2.5-rc1'
expect_failure 'pre-release tag' v1.2.5-rc1 refs/heads/main
expect_failure 'missing tag' v9.9.9 refs/heads/main
expect_failure 'branch name' main refs/heads/main

# A valid annotated tag on a commit outside main must fail closed.
git -C "$repo" switch -q -c side
git -C "$repo" commit -q --allow-empty -m 'test: side-only'
git -C "$repo" tag -a v2.0.0 -m 'v2.0.0'
git -C "$repo" switch -q main
expect_failure 'tag not reachable from main' v2.0.0 refs/heads/main

print -r -- 'release_ref_validation OK'
