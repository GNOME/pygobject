#!/bin/bash

set -e

python --version
virtualenv --python=python _venv
source _venv/bin/activate

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
python setup.py build_tests
xvfb-run -a python -m coverage run tests/runtests.py
