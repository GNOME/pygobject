#!/bin/bash

set -e

python3 --version
python3 -m venv _venv
source _venv/bin/activate

# ccache setup
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
COV_DIR="$(pwd)/coverage"
COV_KEY="${CI_JOB_NAME}"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
mkdir -p "${COV_DIR}"
mkdir -p "${CCACHE_DIR}"

# test
python -m pip install --upgrade pip
python -m pip install pycairo pytest pytest-cov meson ninja

meson setup _build
PYTEST_ADDOPTS="--cov" xvfb-run -a meson test --suite pygobject --timeout-multiplier 4 -C _build -v
python -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"
