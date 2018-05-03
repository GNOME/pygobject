#!/bin/bash

set -e

python --version

PYVER=$(python -c "import sys; sys.stdout.write(''.join(map(str, sys.version_info[:3])))")
PYIMPL=$(python -c "import sys, platform; sys.stdout.write(platform.python_implementation())")
SOURCE_DIR="$(pwd)"
COV_DIR="${SOURCE_DIR}/coverage"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
export G_SLICE="debug-blocks"
export COVERAGE_FILE="${COV_DIR}/.coverage.${PYVER}"
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

mkdir -p "${CCACHE_DIR}"
mkdir -p "${COV_DIR}"

python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install flake8 pytest pytest-faulthandler coverage

export CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror"

# MESON
if [[ "${PYIMPL}" != "PyPy" ]]; then
    # https://github.com/mesonbuild/meson/pull/3445
    /usr/bin/python3 -m pip install --user meson
    export PATH="${HOME}/.local/bin:${PATH}"
    meson _build -Dpython="$(which python)"
    ninja -C _build
    rm -Rf _build
fi;

# CODE QUALITY
python -m flake8

# DOCUMENTATION CHECKS
if [[ "${PYENV_VERSION}" == "2.7.14" ]]; then
    python -m pip install sphinx sphinx_rtd_theme
    python -m sphinx -W -a -E -b html -n docs docs/_build
fi;

# BUILD & TEST AGAIN USING SETUP.PY
python setup.py build_tests
xvfb-run -a python -m coverage run tests/runtests.py

# COLLECT GCOV COVERAGE
lcov --rc lcov_branch_coverage=1 --directory . --capture --output-file \
    "${COV_DIR}/${PYVER}.lcov"
