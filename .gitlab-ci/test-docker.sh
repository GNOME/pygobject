#!/bin/bash

set -e

python --version

SOURCE_DIR="$(pwd)"
COV_DIR="${SOURCE_DIR}/coverage"
COV_KEY="${CI_JOB_NAME_SLUG}"
JUNIT_XML="${SOURCE_DIR}/test-results.xml"

CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror"
MALLOC_CHECK_=3
MALLOC_PERTURB_=$((RANDOM % 255 + 1))
G_SLICE="debug-blocks"
COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
CCACHE_BASEDIR="$(pwd)"
CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
export CFLAGS MALLOC_CHECK_ MALLOC_PERTURB_ G_SLICE COVERAGE_FILE \
       CCACHE_BASEDIR CCACHE_DIR

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1
export MESONPY_EDITABLE_VERBOSE=1

mkdir -p "${CCACHE_DIR}"
mkdir -p "${COV_DIR}"

python -m venv /tmp/venv
# shellcheck disable=SC1091
source /tmp/venv/bin/activate

python -m pip install --upgrade pip
python -m pip install meson meson-python pycairo pytest pytest-cov

# BUILD & TEST
python -m pip install --config-settings=setup-args="-Dtests=true" --no-build-isolation --editable .

# TEST
lcov --config-file .gitlab-ci/lcovrc --directory . --capture --initial --output-file \
    "${COV_DIR}/${CI_JOB_NAME_SLUG}-baseline.lcov"

xvfb-run -a python -m pytest -vs --cov --junit-xml="${JUNIT_XML}"
python -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"

# COLLECT GCOV COVERAGE
lcov --config-file .gitlab-ci/lcovrc --directory . --capture --output-file \
    "${COV_DIR}/${CI_JOB_NAME_SLUG}.lcov"
