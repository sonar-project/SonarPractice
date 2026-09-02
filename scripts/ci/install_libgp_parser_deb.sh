#!/usr/bin/env bash
# Install libgp_parser .debs for the current Ubuntu series (from GitHub Release,
# or build from source when the release assets are missing).
set -euo pipefail

LIBGP_PARSER_TAG="${LIBGP_PARSER_TAG:-v0.2.1}"
# Source fallback ref (packaging lives on main until the next tagged release).
LIBGP_PARSER_GIT_REF="${LIBGP_PARSER_GIT_REF:-main}"
UBUNTU_RELEASE="${UBUNTU_RELEASE:?UBUNTU_RELEASE required (e.g. 25.10)}"
SUFFIX="ubuntu$(echo "${UBUNTU_RELEASE}" | tr -d '.')"
REPO="${LIBGP_PARSER_REPO:-sonar-project/libgp_parser}"
WORKDIR="${LIBGP_PARSER_WORKDIR:-$(pwd)/.ci/libgp_parser-debs}"

mkdir -p "${WORKDIR}"
cd "${WORKDIR}"
rm -f ./*.deb

download_from_release() {
  echo "Trying GitHub release ${LIBGP_PARSER_TAG} (*_${SUFFIX}.deb) from ${REPO}..."
  local api="https://api.github.com/repos/${REPO}/releases/tags/${LIBGP_PARSER_TAG}"
  local json
  if ! json="$(curl -fsSL "${api}")"; then
    echo "Release ${LIBGP_PARSER_TAG} not found (or network error)."
    return 1
  fi

  local urls
  urls="$(printf '%s' "${json}" | sed -n 's/.*"browser_download_url": "\([^"]*\)".*/\1/p' | grep "_${SUFFIX}\\.deb$" || true)"
  if [[ -z "${urls}" ]]; then
    echo "No *_${SUFFIX}.deb assets on release ${LIBGP_PARSER_TAG}."
    return 1
  fi

  while IFS= read -r url; do
    [[ -z "${url}" ]] && continue
    echo "Downloading ${url}"
    curl -fsSL -O "${url}"
  done <<< "${urls}"

  compgen -G "${WORKDIR}/*_${SUFFIX}.deb" >/dev/null
}

build_from_source() {
  local ref="${LIBGP_PARSER_GIT_REF}"
  echo "Building libgp_parser (${ref}) from source for Ubuntu ${UBUNTU_RELEASE}..."
  local src="${WORKDIR}/src"
  rm -rf "${src}"
  git clone --depth 1 --branch "${ref}" \
    "https://github.com/${REPO}.git" "${src}"
  cmake -S "${src}" -B "${src}/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_SHARED_LIBS=ON \
    -DLIBGP_PARSER_BUILD_TESTS=OFF \
    -DLIBGP_PARSER_BUILD_EXAMPLE=OFF
  cmake --build "${src}/build"
  (cd "${src}/build" && cpack -G DEB)
  shopt -s nullglob
  local built=("${src}/build"/*.deb)
  if [[ ${#built[@]} -eq 0 ]]; then
    echo "cpack produced no .deb files — is packaging enabled in ${ref}?" >&2
    exit 1
  fi
  for deb in "${built[@]}"; do
    base="$(basename "${deb}" .deb)"
    cp "${deb}" "${WORKDIR}/${base}_${SUFFIX}.deb"
  done
}

if ! download_from_release; then
  build_from_source
fi

echo "Installing:"
ls -la "${WORKDIR}"/*_"${SUFFIX}".deb
export DEBIAN_FRONTEND=noninteractive
apt-get install -y "${WORKDIR}"/*_"${SUFFIX}".deb
