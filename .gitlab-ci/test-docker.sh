#!/bin/bash

set -e

python --version

PYVER=$(python -c "import sys; sys.stdout.write('.'.join(map(str, sys.version_info[:2])))")
PYIMPL=$(python -c "import sys, platform; sys.stdout.write(platform.python_implementation())")
SOURCE_DIR="$(pwd)"
COV_DIR="${SOURCE_DIR}/coverage"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
export G_SLICE="debug-blocks"
COV_KEY="${CI_JOB_NAME}"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1

mkdir -p "${CCACHE_DIR}"
mkdir -p "${COV_DIR}"

# NB. setuptools is needed, since distutils has been removed from Python 3.12
python -m pip install pycairo flake8 pytest pytest-cov pytest-faulthandler setuptools

# MESON
/usr/bin/python3 -m pip install --user meson
export PATH="${HOME}/.local/bin:${PATH}"
export PKG_CONFIG_PATH="$(python -c 'import sys; sys.stdout.write(sys.prefix)')/lib/pkgconfig"

# CODE QUALITY
python -m flake8

# BUILD & TEST
CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror" meson setup _build -Dpython="$(which python)"

lcov --config-file .gitlab-ci/lcovrc --directory . --capture --initial --output-file \
    "${COV_DIR}/${CI_JOB_NAME}-baseline.lcov"

PYTEST_ADDOPTS="--cov" xvfb-run -a meson test --suite pygobject --timeout-multiplier 4 -C _build -v
python -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"

# COLLECT GCOV COVERAGE
lcov --config-file .gitlab-ci/lcovrc --directory . --capture --output-file \
    "${COV_DIR}/${CI_JOB_NAME}.lcov"
