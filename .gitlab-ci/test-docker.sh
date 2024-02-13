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

python -m pip install pdm

# BUILD
python -m pdm install

# CODE QUALITY
python -m pdm run flake8

# TEST
lcov --config-file .gitlab-ci/lcovrc --directory . --capture --initial --output-file \
    "${COV_DIR}/${CI_JOB_NAME}-baseline.lcov"

xvfb-run -a python -m pdm run pytest -v --cov
python -m pdm run coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"

# COLLECT GCOV COVERAGE
lcov --config-file .gitlab-ci/lcovrc --directory . --capture --output-file \
    "${COV_DIR}/${CI_JOB_NAME}.lcov"
