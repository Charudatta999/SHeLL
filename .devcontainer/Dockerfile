FROM mcr.microsoft.com/devcontainers/base:fedora

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
        which && \
    dnf clean all
