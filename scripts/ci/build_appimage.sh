#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-${GITHUB_REF_NAME:-}}"
VERSION="${VERSION#v}"
ARTIFACT_NAME="SonarPractice-${VERSION}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT}/build"
APPDIR="${ROOT}/AppDir"

rm -rf "${BUILD_DIR}" "${APPDIR}" "${ROOT}/${ARTIFACT_NAME}"*.AppImage

cmake -S "${ROOT}" -B "${BUILD_DIR}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF

cmake --build "${BUILD_DIR}" --target SonarPractice

mkdir -p "${APPDIR}/usr/bin"
cp "${BUILD_DIR}/SonarPractice" "${APPDIR}/usr/bin/"

RUBBERBAND_LIB="$(find "${BUILD_DIR}" -maxdepth 1 -name 'libSonarPractice_Rubberband.so*' -print -quit)"
if [[ -n "${RUBBERBAND_LIB}" ]]; then
  cp "${RUBBERBAND_LIB}" "${APPDIR}/usr/bin/"
fi

cp "${ROOT}/scripts/ci/sonarpractice.appimage.desktop" "${APPDIR}/sonarpractice.desktop"
cp "${ROOT}/assets/svg/icon.svg" "${APPDIR}/sonarpractice.svg"

LINUXDEPLOY="${ROOT}/.ci/linuxdeploy-x86_64.AppImage"
chmod +x "${LINUXDEPLOY}"

export QML_SOURCES_PATHS="${ROOT}/src/ui"
export EXTRA_PLATFORM_PLUGINS="wayland-egl"

"${LINUXDEPLOY}" \
  --appdir "${APPDIR}" \
  --executable "${APPDIR}/usr/bin/SonarPractice" \
  --desktop-file "${APPDIR}/sonarpractice.desktop" \
  --icon-file "${APPDIR}/sonarpractice.svg" \
  --plugin qt \
  --output appimage

mv "${ROOT}/SonarPractice"*.AppImage "${ROOT}/${ARTIFACT_NAME}.AppImage"

echo "Created ${ROOT}/${ARTIFACT_NAME}.AppImage"
