#!/usr/bin/env zsh
# Resolve exactly one private release draft by tag and target commit.
emulate -R zsh
setopt err_exit no_unset pipe_fail

if (( $# != 3 )); then
  print -ru2 -- \
    'usage: resolve-release-draft.zsh <owner/repository> <tag> <commit>'
  exit 2
fi

typeset repository=$1
typeset release_tag=$2
typeset release_commit=$3
typeset releases_json draft_ids_output
typeset -a draft_ids

releases_json=$(gh api --paginate --slurp \
  "repos/$repository/releases?per_page=100")
draft_ids_output=$(jq -r \
  --arg tag "$release_tag" \
  --arg commit "$release_commit" \
  '.[][] | select(
    .draft == true and
    .tag_name == $tag and
    .target_commitish == $commit
  ) | .id' <<< "$releases_json")
draft_ids=( ${(f)draft_ids_output} )

if (( ${#draft_ids[@]} != 1 )); then
  print -ru2 -- \
    "expected exactly one draft for $release_tag at $release_commit; found ${#draft_ids[@]}"
  exit 1
fi

case $draft_ids[1] in
  (''|*[!0-9]*)
    print -ru2 -- "resolved release ID is not numeric: $draft_ids[1]"
    exit 1
    ;;
esac

print -r -- "$draft_ids[1]"
