#!/usr/bin/env bash
# Usage: packaging/make-source-tarball.sh <version> [outdir]
# Writes <outdir>/shell-repl-<version>.tar.gz from the committed tree (git archive).
set -euo pipefail
ver="${1:?version required}"
out="${2:-packaging/arch}"
mkdir -p "$out"
git archive --format=tar.gz --prefix="shell-repl-${ver}/" \
  -o "${out}/shell-repl-${ver}.tar.gz" HEAD
echo "${out}/shell-repl-${ver}.tar.gz"
