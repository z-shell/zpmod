#!/usr/bin/env zsh
# Validate an immutable final-release boundary and print its commit SHA.
emulate -L zsh
set -euo pipefail

release_tag=${1:-}
main_ref=${2:-origin/main}
remote=${3:-}

if [[ ! $release_tag =~ '^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$' ]]; then
  print -ru2 -- "release tag must match vX.Y.Z: ${release_tag:-<empty>}"
  exit 1
fi

tag_ref="refs/tags/$release_tag"
if [[ -n $remote ]]; then
  git fetch --quiet --force --no-tags "$remote" \
    "${tag_ref}:${tag_ref}" \
    "refs/heads/main:${main_ref}" >/dev/null || {
    print -ru2 -- "release refs are not available from $remote: $release_tag"
    exit 1
  }
fi
object_type=$(git cat-file -t "$tag_ref" 2>/dev/null) || {
  print -ru2 -- "release tag does not exist: $release_tag"
  exit 1
}
if [[ $object_type != tag ]]; then
  print -ru2 -- "release tag must be annotated: $release_tag"
  exit 1
fi

release_commit=$(git rev-parse --verify "${tag_ref}^{commit}" 2>/dev/null) || {
  print -ru2 -- "release tag does not resolve to a commit: $release_tag"
  exit 1
}
git rev-parse --verify "${main_ref}^{commit}" >/dev/null 2>&1 || {
  print -ru2 -- "main reference does not exist: $main_ref"
  exit 1
}
if ! git merge-base --is-ancestor "$release_commit" "$main_ref"; then
  print -ru2 -- "$release_tag ($release_commit) is not reachable from $main_ref"
  exit 1
fi

print -r -- "$release_commit"
