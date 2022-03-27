#!/bin/bash

set -e

pacman --noconfirm -Suy

pacman --noconfirm -S --needed \
    "${MINGW_PACKAGE_PREFIX}"-ccache \
    "${MINGW_PACKAGE_PREFIX}"-glib2 \
    "${MINGW_PACKAGE_PREFIX}"-gobject-introspection \
    "${MINGW_PACKAGE_PREFIX}"-gtk3 \
    "${MINGW_PACKAGE_PREFIX}"-libffi \
    "${MINGW_PACKAGE_PREFIX}"-python \
    "${MINGW_PACKAGE_PREFIX}"-python-cairo \
    "${MINGW_PACKAGE_PREFIX}"-python-coverage \
    "${MINGW_PACKAGE_PREFIX}"-python-pip \
    "${MINGW_PACKAGE_PREFIX}"-python-pytest \
    "${MINGW_PACKAGE_PREFIX}"-toolchain \
    git \
    lcov

# ccache setup
export PATH="$MSYSTEM/lib/ccache/bin:$PATH"
mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# coverage setup
export CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror"
COV_DIR="$(pwd)/coverage"
COV_KEY="${CI_JOB_NAME}"
mkdir -p "${COV_DIR}"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"

# FIXME: g_callable_info_free_closure etc
CFLAGS+=" -Wno-error=deprecated-declarations"

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1

python setup.py build_tests

lcov \
    --config-file .gitlab-ci/lcovrc \
    --directory "$(pwd)" \
    --capture --initial --output-file \
    "${COV_DIR}/${COV_KEY}-baseline.lcov"

MSYSTEM= python -m coverage run --context "${COV_KEY}" tests/runtests.py
MSYSTEM= python -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"

lcov \
    --config-file .gitlab-ci/lcovrc \
    --directory "$(pwd)" --capture --output-file \
    "${COV_DIR}/${COV_KEY}.lcov"
