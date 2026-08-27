#!/usr/bin/env zsh
# Static policy checks for release and Pages workflows.
emulate -L zsh
set -euo pipefail

repo_root=${0:A:h:h:h}
release="$repo_root/.github/workflows/release.yml"
docs="$repo_root/.github/workflows/docs.yml"

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

grep -q '^  workflow_dispatch:' "$release" ||
  fail_test 'release workflow lacks the explicit publication gate'
grep -q '^        required: true' "$release" ||
  fail_test 'release tag input is not mandatory'
if grep -q '^  push:' "$release"; then
  fail_test 'release workflow still publishes automatically on tag push'
fi
grep -q 'validate-release-ref.zsh' "$release" ||
  fail_test 'release workflow does not validate the release ref'
grep -q -- '--ctest' "$release" ||
  fail_test 'release workflow does not run the full CTest suite'
grep -q 'needs: validate' "$release" ||
  fail_test 'release builds are not gated by ref validation'
grep -q -- '--verify-tag' "$release" ||
  fail_test 'release publication does not require the remote tag'
grep -q -- '--draft' "$release" ||
  fail_test 'release assets are uploaded directly to a public release'
grep -q 'gh release edit' "$release" ||
  fail_test 'release workflow does not publish a validated draft'
grep -q 'cleanup_draft' "$release" ||
  fail_test 'failed draft validation does not clean up the private release'
grep -q 'draft_id=' "$release" ||
  fail_test 'draft cleanup is not scoped to a release ID'
if grep -q 'gh release delete.*RELEASE_TAG' "$release"; then
  fail_test 'draft cleanup can delete a pre-existing release by tag'
fi
grep -q 'Release tag object moved' "$release" ||
  fail_test 'release publication does not revalidate after the builds'

# Workflow-scope Docs permissions end at the concurrency key.
docs_top_permissions=$(sed -n '/^permissions:/,/^concurrency:/p' "$docs")
[[ $docs_top_permissions == *'contents: read'* ]] ||
  fail_test 'Docs workflow lacks workflow-scope contents: read'
[[ $docs_top_permissions != *'pages: write'* ]] ||
  fail_test 'Docs workflow grants pages: write to PR build jobs'
[[ $docs_top_permissions != *'id-token: write'* ]] ||
  fail_test 'Docs workflow grants id-token: write to PR build jobs'

deploy_block=$(sed -n '/^  deploy:/,$p' "$docs")
[[ $deploy_block == *'pages: write'* ]] ||
  fail_test 'Docs deploy job lacks pages: write'
[[ $deploy_block == *'id-token: write'* ]] ||
  fail_test 'Docs deploy job lacks id-token: write'

print -r -- 'release_workflow_policy OK'
