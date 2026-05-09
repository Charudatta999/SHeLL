FROM quay.io/fedora/fedora:latest

# Set Clang as the default compiler
ENV CC=clang
ENV CXX=clang++

# Create non-root user
RUN dnf install -y sudo && \
    useradd -m -s /bin/bash vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/nopasswd

# Install required C++ build tools and utilities
RUN dnf update -y && \
    dnf install -y \
        clang \
        clang-tools-extra \
        lldb \
        lld \
        make \
        cmake \
        git \
        wget \
        tar \
        which \
        libtool \
        sudo \
        ninja-build \
        mesa-libGL-devel \
        libglvnd-devel \
        libglvnd-opengl-devel \
        libglvnd-glx-devel && \
    dnf clean all