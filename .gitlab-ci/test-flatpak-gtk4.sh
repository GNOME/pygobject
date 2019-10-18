#!/bin/bash

set -e

COV_DIR="$(pwd)/coverage"
export COVERAGE_FILE="${COV_DIR}/.coverage.gtk4"
mkdir -p "${COV_DIR}"

export TEST_GTK_VERSION=4.0
python3 -m pip install --user pytest pytest-faulthandler coverage
python3 setup.py build_tests
python3 -m coverage run tests/runtests.py
chmod -R 777 "${COV_DIR}"