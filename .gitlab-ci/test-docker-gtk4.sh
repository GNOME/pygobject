#!/bin/bash

set -e

# ccache setup
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
COV_DIR="$(pwd)/coverage"
export COVERAGE_FILE="${COV_DIR}/.coverage.${CI_JOB_NAME}"
mkdir -p "${COV_DIR}"
mkdir -p "${CCACHE_DIR}"

# test
python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install pytest pytest-faulthandler coverage
g-ir-inspect Gtk --version=4.0 --print-typelibs
export TEST_GTK_VERSION=4.0
python setup.py build_tests
xvfb-run -a python -m coverage run tests/runtests.py
