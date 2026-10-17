#!/usr/bin/env bash
set -euo pipefail

# Patches CMakeLists.txt project() VERSION from a git tag.
#
# Git tags (SemVer):
#   v0.1.4           → stable release of 0.1.4
#   v0.1.4-rc.1      → prerelease (recommended)
#   v0.1.4-beta.1    → prerelease
#   v0.1.4-r1        → also accepted as prerelease label
#
# CMake only allows numeric major[.minor[.patch[.tweak]]]. A tag like
# v0.1.4-rc.1 therefore becomes project(... VERSION 0.1.4). Artifact names and
# GitHub Releases still use the full SemVer string (without leading v).
#
# Repo keeps 0.0.0 in CMakeLists.txt for local builds; CI runs this before cmake.

RAW="${1:-${GITHUB_REF_NAME:-}}"
RAW="${RAW#v}"

if [[ -z "${RAW}" ]]; then
  echo "Usage: set_version.sh <tag-or-version>" >&2
  exit 1
fi

# Core: 1.2.3 or 1.2.3.4 — optional SemVer prerelease / build metadata after - or +.
if [[ ! "${RAW}" =~ ^([0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?)([-+][0-9A-Za-z.-]+)?$ ]]; then
  echo "Unsupported version format: ${RAW}" >&2
  echo "Expected SemVer like 0.1.4 or 0.1.4-rc.1 (optional leading v)." >&2
  exit 1
fi

CMAKE_VERSION="${BASH_REMATCH[1]}"
FULL_VERSION="${RAW}"
IS_PRERELEASE=false
if [[ "${FULL_VERSION}" == *-* ]]; then
  IS_PRERELEASE=true
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CMAKE_FILE="${ROOT}/CMakeLists.txt"

sed -i "s/^project(SonarPractice VERSION .*/project(SonarPractice VERSION ${CMAKE_VERSION})/" "${CMAKE_FILE}"

echo "Set CMake project version to ${CMAKE_VERSION} (full: ${FULL_VERSION}, prerelease: ${IS_PRERELEASE})"

if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
  {
    echo "cmake_version=${CMAKE_VERSION}"
    echo "full_version=${FULL_VERSION}"
    echo "is_prerelease=${IS_PRERELEASE}"
  } >> "${GITHUB_OUTPUT}"
fi
