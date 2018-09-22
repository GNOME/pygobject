#!/bin/bash

set -e

# ccache setup
mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# test
python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install pytest pytest-faulthandler
g-ir-inspect Gtk --version=4.0 --print-typelibs
export TEST_GTK_VERSION=4.0
xvfb-run -a python setup.py test
