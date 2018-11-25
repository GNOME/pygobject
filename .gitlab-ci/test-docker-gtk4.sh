#!/bin/bash

set -e

# ccache setup
mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# test
python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install pytest pytest-faulthandler coverage
g-ir-inspect Gtk --version=4.0 --print-typelibs
export TEST_GTK_VERSION=4.0
python setup.py build_tests
xvfb-run -a python -m coverage run tests/runtests.py
