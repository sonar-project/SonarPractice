#!/usr/bin/env bash
# Local AppImage build for SonarPractice (Qt 6.11.x from the online installer).
#
# Prerequisites:
#   - Release build of SonarPractice (Qt Creator or cmake)
#   - linuxdeploy + linuxdeploy-plugin-qt in ~/.local/bin/
#   - Optional: APPIMAGE_EXTRACT_AND_RUN=1 if FUSE is unavailable
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QT_DIR="${QT_DIR:-$HOME/Qt/6.11.1/gcc_64}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build/Desktop_Qt_6_11_1_Release}"
APPDIR="${ROOT}/AppDir"
LINUXDEPLOY="${LINUXDEPLOY:-$HOME/.local/bin/linuxdeploy-x86_64.AppImage}"
VERSION="${VERSION:-$(grep -m1 'project(SonarPractice VERSION' "$ROOT/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')}"
ARTIFACT="${ROOT}/SonarPractice-${VERSION}-x86_64.AppImage"

BINARY="${BUILD_DIR}/SonarPractice"
RUBBERBAND_LIB="${BUILD_DIR}/libSonarPractice_Rubberband.so"

if [[ ! -x "${BINARY}" ]]; then
  echo "Binary not found: ${BINARY}" >&2
  echo "Build the Release target first or set BUILD_DIR." >&2
  exit 1
fi

if [[ ! -x "${LINUXDEPLOY}" ]]; then
  echo "linuxdeploy not found: ${LINUXDEPLOY}" >&2
  exit 1
fi

export QMAKE="${QT_DIR}/bin/qmake"
export PATH="${QT_DIR}/bin:${PATH}"
export LD_LIBRARY_PATH="${QT_DIR}/lib:${LD_LIBRARY_PATH:-}"
export QML_SOURCES_PATHS="${ROOT}/src/ui"
export APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}"

# linuxdeploy-plugin-qt copies every SQL driver shipped with Qt. Drivers such as
# libqsqlmimer.so pull in proprietary libs (libmimerapi.so) that are not
# installed locally and are not needed (we only use QSQLITE). Mask them for the
# duration of the build.
QT_SQLDRIVERS="${QT_DIR}/plugins/sqldrivers"
SQL_DRIVER_MASK_DIR=""
restore_sql_drivers() {
  if [[ -n "${SQL_DRIVER_MASK_DIR}" && -d "${SQL_DRIVER_MASK_DIR}" ]]; then
    mv "${SQL_DRIVER_MASK_DIR}"/* "${QT_SQLDRIVERS}/" 2>/dev/null || true
    rmdir "${SQL_DRIVER_MASK_DIR}" 2>/dev/null || true
  fi
}
trap restore_sql_drivers EXIT

if [[ -d "${QT_SQLDRIVERS}" ]]; then
  SQL_DRIVER_MASK_DIR="$(mktemp -d)"
  for driver in libqsqlmimer.so libqsqlmysql.so libqsqlibase.so libqsqloci.so libqsqlodbc.so libqsqlpsql.so; do
    if [[ -f "${QT_SQLDRIVERS}/${driver}" ]]; then
      mv "${QT_SQLDRIVERS}/${driver}" "${SQL_DRIVER_MASK_DIR}/"
    fi
  done
fi

rm -rf "${APPDIR}" "${ROOT}"/SonarPractice*.AppImage
mkdir -p "${APPDIR}/usr/bin"
cp "${BINARY}" "${APPDIR}/usr/bin/"
if [[ -f "${RUBBERBAND_LIB}" ]]; then
  cp "${RUBBERBAND_LIB}" "${APPDIR}/usr/bin/"
fi

cp "${ROOT}/scripts/ci/sonarpractice.appimage.desktop" "${APPDIR}/sonarpractice.desktop"
cp "${ROOT}/assets/svg/icon.svg" "${APPDIR}/sonarpractice.svg"

"${LINUXDEPLOY}" \
  --appdir "${APPDIR}" \
  --executable "${APPDIR}/usr/bin/SonarPractice" \
  --desktop-file "${APPDIR}/sonarpractice.desktop" \
  --icon-file "${APPDIR}/sonarpractice.svg" \
  --plugin qt \
  --output appimage

mv "${ROOT}/SonarPractice"*.AppImage "${ARTIFACT}"
echo "Created ${ARTIFACT}"
