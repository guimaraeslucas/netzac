#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "build/build.ninja" ]; then
    meson setup build
fi

ninja -C build
exec ./build/shell/netzac/zacos9-netzac "$@"
