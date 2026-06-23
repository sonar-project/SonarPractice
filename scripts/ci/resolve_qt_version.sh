#!/usr/bin/env bash
# Print the newest Qt 6.x desktop version available via aqt for the given host.
# Usage: resolve_qt_version.sh [linux|windows]
set -euo pipefail

HOST="${1:-linux}"

if command -v python3 >/dev/null 2>&1; then
  PYTHON=python3
elif command -v python >/dev/null 2>&1; then
  PYTHON=python
else
  echo "Python not found (need python3 or python on PATH)" >&2
  exit 1
fi

"${PYTHON}" -m pip install --disable-pip-version-check -q 'aqtinstall==3.3.*'
"${PYTHON}" -m aqt list-qt "${HOST}" desktop --spec "6" --latest-version
