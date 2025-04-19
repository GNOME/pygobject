#!/bin/bash

set -e

pacman --noconfirm -Suy

pacman --noconfirm -S --needed \
    "${MINGW_PACKAGE_PREFIX}"-ccache \
    "${MINGW_PACKAGE_PREFIX}"-glib2 \
    "${MINGW_PACKAGE_PREFIX}"-gobject-introspection \
    "${MINGW_PACKAGE_PREFIX}"-gtk3 \
    "${MINGW_PACKAGE_PREFIX}"-libffi \
    "${MINGW_PACKAGE_PREFIX}"-meson \
    "${MINGW_PACKAGE_PREFIX}"-ninja \
    "${MINGW_PACKAGE_PREFIX}"-python \
    "${MINGW_PACKAGE_PREFIX}"-python-cairo \
    "${MINGW_PACKAGE_PREFIX}"-python-pip \
    "${MINGW_PACKAGE_PREFIX}"-python-pytest \
    "${MINGW_PACKAGE_PREFIX}"-python-pytest-cov \
    "${MINGW_PACKAGE_PREFIX}"-toolchain \
    git \
    lcov

# ccache setup
export PATH="$MSYSTEM/lib/ccache/bin:$PATH"
mkdir -p _ccache
CCACHE_BASEDIR="$(pwd)"
export CCACHE_BASEDIR
CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
export CCACHE_DIR

# coverage setup
COV_DIR="$(pwd)/coverage"
COV_KEY="${CI_JOB_NAME_SLUG}"
mkdir -p "${COV_DIR}"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"

# Test results
JUNIT_XML="test-results.xml"

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1


MSYSTEM='' CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror" meson setup _build

lcov \
    --config-file .gitlab-ci/lcovrc \
    --directory "$(pwd)" --capture --initial --output-file \
    "${COV_DIR}/${COV_KEY}-baseline.lcov"

MSYSTEM='' PYTEST_ADDOPTS="--cov -sv --junit-xml=${JUNIT_XML}" meson test --suite pygobject --timeout-multiplier 4 -C _build -v
MSYSTEM='' python -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"

lcov \
    --config-file .gitlab-ci/lcovrc \
    --directory "$(pwd)" --capture --output-file \
    "${COV_DIR}/${COV_KEY}.lcov"
