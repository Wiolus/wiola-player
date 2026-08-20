#!/usr/bin/env bash
set -euo pipefail

# Builds a static Qt for MinGW-w64 and installs it where the windows preset can find it. Only
# qtbase is built; the window uses nothing else.

# The install prefix is asked for rather than guessed: this writes gigabytes, and where they land
# is the caller's to decide. Everything written lives under it, so one path is all there is to
# remove afterwards.
if [ $# -ne 1 ]; then
    echo "usage: $0 <install-prefix>" >&2
    exit 2
fi

prefix="$1"
work="$prefix/.work"

# Cross-builds borrow moc, rcc and uic from the host, and Qt refuses a version mismatch.
version="$(qmake6 -query QT_VERSION)"
host_cmake_dir="$(dirname "$(qmake6 -query QT_INSTALL_LIBS)")/$(uname -m)-linux-gnu/cmake"

if [ ! -d "$host_cmake_dir/Qt6" ]; then
    host_cmake_dir="$(qmake6 -query QT_INSTALL_LIBS)/cmake"
fi

source_dir="$work/qtbase-everywhere-src-$version"
mkdir -p "$work"

if [ ! -d "$source_dir" ]; then
    curl -SL --output-dir "$work" -o qtbase.tar.xz \
        "https://download.qt.io/archive/qt/${version%.*}/$version/submodules/qtbase-everywhere-src-$version.tar.xz"
    tar xf "$work/qtbase.tar.xz" -C "$work"
    rm "$work/qtbase.tar.xz"
fi

cmake -G Ninja -S "$source_dir" -B "$work/build" \
    -DCMAKE_TOOLCHAIN_FILE="$(git rev-parse --show-toplevel)/cmake/mingw-w64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_SHARED_LIBS=OFF \
    -DQT_HOST_PATH=/usr \
    -DQT_HOST_PATH_CMAKE_DIR="$host_cmake_dir" \
    -DQT_BUILD_EXAMPLES=OFF \
    -DQT_BUILD_TESTS=OFF \
    -DFEATURE_sql=OFF \
    -DFEATURE_dbus=OFF \
    -DFEATURE_testlib=OFF

cmake --build "$work/build" --parallel
cmake --install "$work/build"

echo "Qt $version installed to $prefix; export QT_MINGW_ROOT=$prefix"
