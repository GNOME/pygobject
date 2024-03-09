#!/bin/bash

set -e

python --version

PYVER=$(python -c "import sys; sys.stdout.write('.'.join(map(str, sys.version_info[:2])))")
PYIMPL=$(python -c "import sys, platform; sys.stdout.write(platform.python_implementation())")
SOURCE_DIR="$(pwd)"
COV_DIR="${SOURCE_DIR}/coverage"
COV_KEY="${CI_JOB_NAME}"

export CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
export G_SLICE="debug-blocks"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1
export MESONPY_EDITABLE_VERBOSE=1

mkdir -p "${CCACHE_DIR}"
mkdir -p "${COV_DIR}"

python -m venv /tmp/venv
source /tmp/venv/bin/activate

# XXX: meson tries to use this for some reason, but it's not there by default
mkdir -p /tmp/venv/include/pypy3.9

python -m pip install --upgrade pip
python -m pip install flake8 meson meson-python pycairo

# CODE QUALITY
python -m flake8

# BUILD & TEST
python -m pip install --config-settings=setup-args="-Dtests=true" --no-build-isolation --editable '.[dev]'

lcov --config-file .gitlab-ci/lcovrc --directory . --capture --initial --output-file \
    "${COV_DIR}/${CI_JOB_NAME}-baseline.lcov"

xvfb-run -a python -m pytest -v --cov
python -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"

# COLLECT GCOV COVERAGE
lcov --config-file .gitlab-ci/lcovrc --directory . --capture --output-file \
    "${COV_DIR}/${CI_JOB_NAME}.lcov"
