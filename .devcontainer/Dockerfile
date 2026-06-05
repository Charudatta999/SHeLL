FROM quay.io/fedora/fedora:latest

# =============================================================================
# Compiler Configuration
# =============================================================================

ENV CC=clang
ENV CXX=clang++

# =============================================================================
# Optional Build Mode
# =============================================================================
#
# Supported:
#   static
#   shared
#   both
#

ARG LIB_TYPE=both

ENV LIB_TYPE=${LIB_TYPE}

# =============================================================================
# Install Tooling + System Dependencies
# =============================================================================

RUN dnf update -y && \
    dnf install -y \
        sudo \
        clang \
        clang-tools-extra \
        llvm \
        lld \
        gcc \
        gcc-c++ \
        lldb \
        make \
        cmake \
        meson \
        ninja-build \
        pkgconf \
        git \
        wget \
        curl \
        tar \
        which \
        file \
        patch \
        autoconf \
        automake \
        libtool \
        python3 \
        python3-pip \
        python3-setuptools \
        python3-wheel \
        gtest-devel \
        gmock-devel \
        bison \
        flex \
        gettext \
        libX11-devel \
        libXext-devel \
        libXcursor-devel \
        libXi-devel \
        libXrandr-devel \
        libXinerama-devel \
        libXScrnSaver-devel \
        wayland-devel \
        wayland-protocols-devel \
        libxkbcommon-devel \
        mesa-libGL-devel && \
    dnf clean all

# =============================================================================
# Python Tooling
# =============================================================================

RUN pip3 install --no-cache-dir glad2

# =============================================================================
# Create vscode User
# =============================================================================

RUN useradd -m -s /bin/bash vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode

# =============================================================================
# Workspace
# =============================================================================

RUN mkdir -p \
    /workspace \
    /workspaces/SHeLL

# =============================================================================
# Environment
# =============================================================================

ENV PATH=/home/vscode/.local/bin:${PATH}

# =============================================================================
# Ownership
# =============================================================================

RUN chown -R vscode:vscode \
    /workspace \
    /workspaces

# =============================================================================
# Switch User
# =============================================================================

USER vscode

# =============================================================================
# Workspace
# =============================================================================

WORKDIR /workspaces/SHeLL

# =============================================================================
# Keep Container Alive
# =============================================================================

CMD ["sleep", "infinity"]