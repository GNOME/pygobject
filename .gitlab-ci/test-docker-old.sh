#!/bin/bash

set -e

python --version
virtualenv --python=python _venv
source _venv/bin/activate

# ccache setup
mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# test
python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install pytest pytest-faulthandler coverage
python setup.py build_tests
xvfb-run -a python -m coverage run tests/runtests.py
