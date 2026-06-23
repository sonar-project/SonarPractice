#!/usr/bin/env bash
set -euo pipefail

# Patches CMakeLists.txt project() version from a git tag (e.g. v1.2.3 -> 1.2.3).
# The repo keeps 0.0.0 in CMakeLists.txt for local builds; CI runs this before cmake.
VERSION="${1:-${GITHUB_REF_NAME:-}}"
VERSION="${VERSION#v}"

if [[ -z "${VERSION}" ]]; then
  echo "Usage: set_version.sh <tag-or-version>" >&2
  exit 1
fi

if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+([.-][0-9A-Za-z.-]+)?$ ]]; then
  echo "Unsupported version format: ${VERSION}" >&2
  exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CMAKE_FILE="${ROOT}/CMakeLists.txt"

sed -i "s/^project(SonarPractice VERSION .*/project(SonarPractice VERSION ${VERSION})/" "${CMAKE_FILE}"

echo "Set project version to ${VERSION} in CMakeLists.txt"
