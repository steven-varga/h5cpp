#!/usr/bin/env bash
# docs-build.sh — derive the project version, build Doxygen, post-process.
#
# Doxyfile's PROJECT_NUMBER reads H5CPP_DOXYGEN_VERSION from the
# environment (via Doxygen's $(VAR) substitution). This script computes
# that value from:
#
#   1. The most recent git tag matching v<n>.<n>.<n>  (preferred — tracks
#      actual releases).
#   2. The H5CPP_PROJECT_VERSION constant in the top-level CMakeLists.txt
#      (fallback for tarball builds with no git history).
#
# Both candidates produce the same string in normal operation because
# CMakeLists.txt also derives from `git describe`; the CMake constant
# only acts as a fallback.
#
# Used by:
#   - the user, locally:  scripts/docs-build.sh
#   - CI (docs.yml):      same invocation, run after actions/checkout.

set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

# CMakeLists.txt is the authoritative source of the current dev version.
# It carries H5CPP_PROJECT_VERSION as a literal "MAJOR.MINOR.PATCH" and
# bumps with each release cycle.
ver=$(grep -oE 'set\(H5CPP_PROJECT_VERSION "[A-Za-z0-9._+-]+"' CMakeLists.txt \
      | head -1 \
      | sed -E 's/.*"([^"]+)".*/\1/')

# If HEAD is sitting exactly on a release tag (post-checkout in CI from a
# tag, or local `git checkout v1.12.x`), prefer the tag — it captures
# what the user actually wants to display: the released version. The
# pattern requires a strict v<n>.<n>.<n> form to skip legacy tags like
# v1.10.4-6 that don't follow semver.
if git rev-parse --git-dir >/dev/null 2>&1; then
  exact=$(git tag --points-at HEAD 2>/dev/null | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+$' | head -1 || true)
  if [[ -n "$exact" ]]; then
    ver="${exact#v}"
  fi
fi

# Absolute last-ditch fallback so the field never renders empty.
ver="${ver:-1.12.7}"

export H5CPP_DOXYGEN_VERSION="v${ver}"
echo "docs-build: PROJECT_NUMBER = $H5CPP_DOXYGEN_VERSION"

( cd doxy && doxygen Doxyfile )

"$repo_root/scripts/docs-postprocess.sh" doxy/docs/doxygen/html

echo "docs-build: generated doxy/docs/doxygen/html/"
