#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
APPDIR="${ROOT}/AppDir"
QT_DIR="${QT_DIR:-${QT_ROOT_DIR:-}}"
LINUXDEPLOY="${LINUXDEPLOY:-${ROOT}/.ci/linuxdeploy-x86_64.AppImage}"

VERSION="${1:-${GITHUB_REF_NAME:-}}"
VERSION="${VERSION#v}"
if [[ -z "${VERSION}" ]]; then
  VERSION="$(grep -m1 'project(SonarPractice VERSION' "${ROOT}/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
fi
ARTIFACT_NAME="SonarPractice-${VERSION}-x86_64"

if [[ -z "${QT_DIR}" || ! -d "${QT_DIR}" ]]; then
  echo "Qt installation not found. Set QT_DIR or QT_ROOT_DIR." >&2
  exit 1
fi

if ! compgen -G "${QT_DIR}/lib/libicui18n.so*" >/dev/null; then
  echo "Qt ICU libraries missing under ${QT_DIR}/lib (incomplete aqt install)." >&2
  exit 1
fi

if [[ ! -x "${LINUXDEPLOY}" ]]; then
  echo "linuxdeploy not found: ${LINUXDEPLOY}" >&2
  exit 1
fi

export QMAKE="${QT_DIR}/bin/qmake"
export PATH="${QT_DIR}/bin:${QT_DIR}/libexec:${PATH}"
export LD_LIBRARY_PATH="${QT_DIR}/lib:${LD_LIBRARY_PATH:-}"

# Qt build tools in libexec (qmlimportscanner, etc.) need Qt's bundled ICU at runtime.
if command -v patchelf >/dev/null 2>&1; then
  for _qt_tool_dir in "${QT_DIR}/libexec" "${QT_DIR}/bin"; do
    [[ -d "${_qt_tool_dir}" ]] || continue
    for _qt_tool in "${_qt_tool_dir}/"*; do
      [[ -f "${_qt_tool}" && -x "${_qt_tool}" ]] || continue
      file "${_qt_tool}" | grep -q ELF || continue
      patchelf --set-rpath '$ORIGIN/../lib' "${_qt_tool}" 2>/dev/null || true
    done
  done
fi

export QML_SOURCES_PATHS="${ROOT}/src/ui"
export APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}"
export STRIP="${STRIP:-true}"

# linuxdeploy-plugin-qt bundles every SQL driver from the Qt install. Unused drivers
# such as libqsqlmimer.so require proprietary libs (libmimerapi.so). We only use QSQLITE.
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

rm -rf "${BUILD_DIR}" "${APPDIR}" "${ROOT}/${ARTIFACT_NAME}"*.AppImage

cmake -S "${ROOT}" -B "${BUILD_DIR}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${QT_DIR}" \
  -DBUILD_TESTING=OFF

cmake --build "${BUILD_DIR}" --target SonarPractice

mkdir -p "${APPDIR}/usr/bin"
cp "${BUILD_DIR}/SonarPractice" "${APPDIR}/usr/bin/"

RUBBERBAND_LIB="$(find "${BUILD_DIR}" -maxdepth 1 -name 'libSonarPractice_RubberbandLib.so*' -print -quit)"
if [[ -n "${RUBBERBAND_LIB}" ]]; then
  cp "${RUBBERBAND_LIB}" "${APPDIR}/usr/bin/"
fi

cp "${ROOT}/scripts/ci/sonarpractice.appimage.desktop" "${APPDIR}/sonarpractice.desktop"
cp "${ROOT}/assets/svg/icon.svg" "${APPDIR}/sonarpractice.svg"

chmod +x "${LINUXDEPLOY}"
PLUGIN="${ROOT}/.ci/linuxdeploy-plugin-qt-x86_64.AppImage"
if [[ -f "${PLUGIN}" ]]; then
  chmod +x "${PLUGIN}"
fi

"${LINUXDEPLOY}" \
  --appdir "${APPDIR}" \
  --executable "${APPDIR}/usr/bin/SonarPractice" \
  --desktop-file "${APPDIR}/sonarpractice.desktop" \
  --icon-file "${APPDIR}/sonarpractice.svg" \
  --plugin qt \
  --output appimage

mv "${ROOT}/SonarPractice"*.AppImage "${ROOT}/${ARTIFACT_NAME}.AppImage"

echo "Created ${ROOT}/${ARTIFACT_NAME}.AppImage"
