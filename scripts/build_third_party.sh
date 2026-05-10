#!/bin/bash
set -e

# Paths
INSTALL_DIR=${THIRD_PARTY_DIR}
THIRD_PARTY_DIR_SRC="${THIRD_PARTY_DIR}/Library-sources"

echo "Building third-party libraries using Clang..."
export CC=clang
export CXX=clang++

mkdir -p "$INSTALL_DIR"

# 1. Build SDL2
if [ -d "$THIRD_PARTY_DIR_SRC/SDL" ]; then
    echo "======================================"
    echo " Building SDL2..."
    echo "======================================"
    cd "$THIRD_PARTY_DIR_SRC/SDL"
    cmake -B build -S . -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" -DCMAKE_BUILD_TYPE=Release -DSDL_STATIC=ON -DSDL_SHARED=ON
    cmake --build build -j$(nproc)
    cmake --install build
fi

# 2. Build FreeType
if [ -d "$THIRD_PARTY_DIR_SRC/freetype" ]; then
    echo "======================================"
    echo " Building FreeType..."
    echo "======================================"
    cd "$THIRD_PARTY_DIR_SRC/freetype"
    cmake -B build -S . -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j$(nproc)
    cmake --install build
fi

# 3. Build HarfBuzz
if [ -d "$THIRD_PARTY_DIR_SRC/harfbuzz" ]; then
    echo "======================================"
    echo " Building HarfBuzz..."
    echo "======================================"
    cd "$THIRD_PARTY_DIR_SRC/harfbuzz"
    cmake -B build -S . -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" -DCMAKE_BUILD_TYPE=Release -DHB_HAVE_FREETYPE=ON -DCMAKE_PREFIX_PATH="$INSTALL_DIR"
    cmake --build build -j$(nproc)
    cmake --install build
fi

# 4. Build libvterm
if [ -d "$THIRD_PARTY_DIR_SRC/libvterm" ]; then
    echo "======================================"
    echo " Building libvterm..."
    echo "======================================"
    cd "$THIRD_PARTY_DIR_SRC/libvterm"
    make clean || true
    make PREFIX="$INSTALL_DIR" -j$(nproc)
    make install PREFIX="$INSTALL_DIR"
fi

# 5. Build RapidJSON (Header-only)
if [ ! -d "$THIRD_PARTY_DIR_SRC/rapidjson" ]; then
    echo "======================================"
    echo " Downloading RapidJSON..."
    echo "======================================"
    cd "$THIRD_PARTY_DIR_SRC"
    wget -qO rapidjson.tar.gz https://github.com/Tencent/rapidjson/archive/refs/heads/master.tar.gz
    tar -xzf rapidjson.tar.gz
    mv rapidjson-master rapidjson
    rm rapidjson.tar.gz
fi
echo "======================================"
echo " Installing RapidJSON headers..."
echo "======================================"
mkdir -p "$INSTALL_DIR/include"
cp -r "$THIRD_PARTY_DIR_SRC/rapidjson/include/rapidjson" "$INSTALL_DIR/include/"

# 6. Install Glad
if [ -d "$THIRD_PARTY_DIR_SRC/glad" ]; then
    echo "======================================"
    echo " Installing Glad headers and source..."
    echo "======================================"
    mkdir -p "$INSTALL_DIR/include"
    mkdir -p "$INSTALL_DIR/src/glad"
    cp -r "$THIRD_PARTY_DIR_SRC/glad/include/"* "$INSTALL_DIR/include/"
    cp -r "$THIRD_PARTY_DIR_SRC/glad/src/"* "$INSTALL_DIR/src/glad/"
fi

echo "======================================"
echo "Done! All third-party libraries built and installed to: $INSTALL_DIR"
echo "You can now configure your main project using:"
echo "cmake -B build -DCMAKE_PREFIX_PATH=\"$INSTALL_DIR\""
