# =============================================================================
# CI image — OEL 8 + Clang, builds shell_core + tests (no GUI).
# Mirrors .github/workflows/ci.yml so CI is reproducible locally:
#
#   podman build -f .devcontainer/ci.Dockerfile -t shell-ci .
#
# The build itself runs the tests (ctest) as a build step, so a successful
# image build == green CI.
# =============================================================================
FROM oraclelinux:8

ENV CC=clang CXX=clang++

# ── Toolchain: C++20-capable clang, cmake, git ──────────────────────────────
RUN dnf -y install oracle-epel-release-el8 || dnf -y install epel-release; \
    dnf -y install llvm-toolset clang cmake make git rapidjson-devel && \
    dnf clean all

# ── GoogleTest + GMock from source (no EL8 package) ─────────────────────────
RUN git clone --depth 1 --branch v1.17.0 \
        https://github.com/google/googletest.git /tmp/gt && \
    cmake -S /tmp/gt -B /tmp/gt/build \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_GMOCK=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/gt/build -j"$(nproc)" && \
    cmake --install /tmp/gt/build && \
    rm -rf /tmp/gt

WORKDIR /src
COPY . /src

# ── Configure → build tests → run them (build fails if tests fail) ──────────
RUN cmake -S /src -B /tmp/out \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_GUI=OFF \
        -DBUILD_TESTING=ON \
        -DBUILD_MCP=OFF \
        -DENABLE_SANITIZERS=OFF && \
    cmake --build /tmp/out --target shell_tests -j"$(nproc)" && \
    ctest --test-dir /tmp/out --output-on-failure
