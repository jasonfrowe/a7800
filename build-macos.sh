#!/bin/sh
# Build mame64 natively on macOS (arm64 on Apple Silicon, x86_64 on Intel)
# against Homebrew's SDL2 (brew install sdl2).
#   ./build-macos.sh            incremental build
#   ./build-macos.sh clean      remove build/ first
set -e
cd "$(dirname "$0")"
[ "$1" = clean ] && rm -rf build
USE_LIBSDL=1 PYTHON_EXECUTABLE=python3 NO_USE_MIDI=1 NO_USE_BGFX=1 \
  make macosx_x64_clang -j"$(sysctl -n hw.ncpu)"
file mame64
