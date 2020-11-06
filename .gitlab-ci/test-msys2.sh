#!/bin/bash

set -e

# skip the fontconfig cache, it's slooowww
export MSYS2_FC_CACHE_SKIP=1
export PANGOCAIRO_BACKEND=win32

export PATH="/c/msys64/$MSYSTEM/bin:$PATH"
if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export MSYS2_ARCH="i686"
else
    export MSYS2_ARCH="x86_64"
fi

pacman --noconfirm -Suy

pacman --noconfirm -S --needed \
    base-devel \
    lcov \
    mingw-w64-$MSYS2_ARCH-toolchain \
    mingw-w64-$MSYS2_ARCH-ccache \
    mingw-w64-$MSYS2_ARCH-$PYTHON-cairo \
    mingw-w64-$MSYS2_ARCH-$PYTHON \
    mingw-w64-$MSYS2_ARCH-$PYTHON-pip \
    mingw-w64-$MSYS2_ARCH-$PYTHON-pytest \
    mingw-w64-$MSYS2_ARCH-$PYTHON-coverage \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-libffi \
    mingw-w64-$MSYS2_ARCH-glib2 \
    mingw-w64-$MSYS2_ARCH-gtk3 \
    git

# ccache setup
export PATH="$MSYSTEM/lib/ccache/bin:$PATH"
mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# coverage setup
export CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror"
PYVER=$($PYTHON -c "import sys; sys.stdout.write(''.join(map(str, sys.version_info[:3])))")
COV_DIR="$(pwd)/coverage"
COV_KEY="${MSYSTEM}.${PYVER}"
mkdir -p "${COV_DIR}"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1

$PYTHON setup.py build_tests

lcov \
    --config-file .gitlab-ci/lcovrc \
    --directory "$(pwd)" \
    --capture --initial --output-file \
    "${COV_DIR}/${COV_KEY}-baseline.lcov"

MSYSTEM= $PYTHON -m coverage run --context "${COV_KEY}" tests/runtests.py

lcov \
    --config-file .gitlab-ci/lcovrc \
    --directory "$(pwd)" --capture --output-file \
    "${COV_DIR}/${COV_KEY}.lcov"
