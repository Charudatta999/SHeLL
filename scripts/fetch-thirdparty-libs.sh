#!/usr/bin/env bash

set -euo pipefail

# =============================================================================
# Paths
# =============================================================================

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

THIRD_PARTY_SRC="${ROOT_DIR}/third_party/Library-sources"

mkdir -p "${THIRD_PARTY_SRC}"

cd "${THIRD_PARTY_SRC}"

# =============================================================================
# Hardcoded Versions
# =============================================================================

SDL_VERSION="release-2.32.8"
FREETYPE_VERSION="VER-2-13-3"
HARFBUZZ_VERSION="11.2.1"

# Keep rolling/master intentionally
LIBVTERM_VERSION="nvim"
RAPIDJSON_REF="master"

# =============================================================================
# Helpers
# =============================================================================

log_section()
{
    echo
    echo "======================================"
    echo " $1"
    echo "======================================"
}

clone_git_repo()
{
    local repo_url="$1"
    local branch="$2"
    local dir_name="$3"

    if [ -d "${dir_name}" ]; then
        echo "${dir_name} already exists."
        return
    fi

    echo "Cloning ${dir_name} (${branch})..."

    git clone \
        --depth 1 \
        --branch "${branch}" \
        "${repo_url}" \
        "${dir_name}"
}

# =============================================================================
# Start
# =============================================================================

log_section "Fetching Third-Party Dependencies"

# =============================================================================
# SDL2
# =============================================================================

clone_git_repo \
    "https://github.com/libsdl-org/SDL.git" \
    "${SDL_VERSION}" \
    "SDL"

# =============================================================================
# FreeType
# =============================================================================

clone_git_repo \
    "https://gitlab.freedesktop.org/freetype/freetype.git" \
    "${FREETYPE_VERSION}" \
    "freetype"

# =============================================================================
# HarfBuzz
# =============================================================================

clone_git_repo \
    "https://github.com/harfbuzz/harfbuzz.git" \
    "${HARFBUZZ_VERSION}" \
    "harfbuzz"

# =============================================================================
# libvterm
# =============================================================================

clone_git_repo \
    "https://github.com/neovim/libvterm.git" \
    "${LIBVTERM_VERSION}" \
    "libvterm"

# =============================================================================
# RapidJSON
# =============================================================================

clone_git_repo \
    "https://github.com/Tencent/rapidjson.git" \
    "${RAPIDJSON_REF}" \
    "rapidjson"

# =============================================================================
# Glad
# =============================================================================

if [ ! -d "glad" ]; then
    log_section "Generating Glad"

    if command -v pip3 >/dev/null 2>&1; then
        pip3 install --user glad2

        python3 -m glad \
            --api gl:core=3.3 \
            --out-path glad \
            c
    else
        echo "ERROR: python3/pip3 required for Glad generation." >&2
        exit 1
    fi
else
    echo "glad already exists."
fi

# =============================================================================
# Mesa OpenGL Headers
# =============================================================================

if [ ! -d "mesa-headers" ]; then
    log_section "Fetching Mesa OpenGL Headers"

    mkdir -p mesa-headers/GL

    pushd mesa-headers/GL >/dev/null

    wget -qO gl.h \
        https://gitlab.freedesktop.org/mesa/mesa/-/raw/main/include/GL/gl.h

    wget -qO glext.h \
        https://gitlab.freedesktop.org/mesa/mesa/-/raw/main/include/GL/glext.h

    wget -qO glcorearb.h \
        https://gitlab.freedesktop.org/mesa/mesa/-/raw/main/include/GL/glcorearb.h

    wget -qO glx.h \
        https://gitlab.freedesktop.org/mesa/mesa/-/raw/main/include/GL/glx.h

    wget -qO glxext.h \
        https://gitlab.freedesktop.org/mesa/mesa/-/raw/main/include/GL/glxext.h

    popd >/dev/null
else
    echo "mesa-headers already exist."
fi

# =============================================================================
# Done
# =============================================================================

log_section "Done"

echo "All dependencies fetched to:"
echo "  ${THIRD_PARTY_SRC}"

echo
echo "Next step:"
echo "  bash scripts/build_third_party.sh"