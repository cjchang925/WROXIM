#!/bin/bash
#
# Fetches and builds this project's two vendored C++ dependencies:
#   - SystemC 3.0.2 (+ TLM 2.0.6), into hybrid/systemc-3.0.2
#   - yaml-cpp 0.8.0, into hybrid/opt/local/yaml-cpp
#
# These directories are not tracked in git (see .gitignore) because they are
# large, machine-specific build products of third-party source. This script
# reproduces the exact layout hybrid/bin/Makefile expects (SYSTEMC/YAML vars).
#
# Idempotent: re-running skips a dependency if it's already built.
#
# Prerequisites (Ubuntu/Debian): build-essential, cmake, git, wget
#   sudo apt-get install -y build-essential cmake git wget
#
# SystemC 3.0.2 requires C++17 and builds only via CMake (the old autotools
# ./configure flow used by SystemC 2.3.1 was dropped upstream). We install
# with -DINSTALL_TO_LIB_TARGET_ARCH_DIR=ON so the libraries land in
# lib-linux64/ (matching the lib-* convention hybrid/bin/Makefile expects)
# instead of CMake's default plain lib/.

set -euo pipefail

HYBRID_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SYSTEMC_VERSION="3.0.2"
SYSTEMC_DIR="${HYBRID_DIR}/systemc-${SYSTEMC_VERSION}"

YAML_CPP_TAG="0.8.0"
YAML_CPP_DIR="${HYBRID_DIR}/opt/local/yaml-cpp"

# --- SystemC ---
if [ -d "${SYSTEMC_DIR}/lib-linux64" ] || compgen -G "${SYSTEMC_DIR}/lib-*" >/dev/null 2>&1; then
    echo "[setup_dependencies] SystemC ${SYSTEMC_VERSION} already built at ${SYSTEMC_DIR}, skipping."
else
    echo "[setup_dependencies] Fetching SystemC ${SYSTEMC_VERSION}..."
    TMP_DIR="$(mktemp -d)"
    wget -O "${TMP_DIR}/systemc-${SYSTEMC_VERSION}.tar.gz" \
        "https://github.com/accellera-official/systemc/archive/refs/tags/${SYSTEMC_VERSION}.tar.gz"
    tar -xzf "${TMP_DIR}/systemc-${SYSTEMC_VERSION}.tar.gz" -C "${TMP_DIR}"

    echo "[setup_dependencies] Building SystemC ${SYSTEMC_VERSION}..."
    (
        cd "${TMP_DIR}/systemc-${SYSTEMC_VERSION}"
        mkdir -p build
        cd build
        cmake .. \
            -DCMAKE_INSTALL_PREFIX="${SYSTEMC_DIR}" \
            -DINSTALL_TO_LIB_TARGET_ARCH_DIR=ON
        make -j"$(nproc)"
        make install
    )
    rm -rf "${TMP_DIR}"
fi

# --- yaml-cpp ---
if [ -f "${YAML_CPP_DIR}/lib/libyaml-cpp.a" ]; then
    echo "[setup_dependencies] yaml-cpp already built at ${YAML_CPP_DIR}, skipping."
else
    echo "[setup_dependencies] Fetching yaml-cpp ${YAML_CPP_TAG}..."
    mkdir -p "$(dirname "${YAML_CPP_DIR}")"
    rm -rf "${YAML_CPP_DIR}"
    git clone --branch "${YAML_CPP_TAG}" --depth 1 https://github.com/jbeder/yaml-cpp "${YAML_CPP_DIR}"

    echo "[setup_dependencies] Building yaml-cpp ${YAML_CPP_TAG}..."
    mkdir -p "${YAML_CPP_DIR}/lib"
    (
        cd "${YAML_CPP_DIR}/lib"
        cmake ..
        make -j"$(nproc)"
    )
fi

echo "[setup_dependencies] Done. Build the simulator with: cd bin && make"
