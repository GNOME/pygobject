#!/bin/bash

set -e

COV_DIR="$(pwd)/coverage"
COV_KEY="gtk4"
export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
mkdir -p "${COV_DIR}"

export TEST_GTK_VERSION=4.0
python3 -m venv _venv
. _venv/bin/activate
python3 -m pip install pytest pytest-faulthandler coverage
python3 setup.py build_tests
python3 -m coverage run --context "${COV_KEY}" tests/runtests.py
python3 -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"
chmod -R 777 "${COV_DIR}"
