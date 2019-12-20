#!/bin/bash

set -e

python --version

PYVER=$(python -c "import sys; sys.stdout.write('.'.join(map(str, sys.version_info[:2])))")
PYIMPL=$(python -c "import sys, platform; sys.stdout.write(platform.python_implementation())")
SOURCE_DIR="$(pwd)"
COV_DIR="${SOURCE_DIR}/coverage"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
export G_SLICE="debug-blocks"
export COVERAGE_FILE="${COV_DIR}/.coverage.${CI_JOB_NAME}"
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# https://docs.python.org/3/using/cmdline.html#envvar-PYTHONDEVMODE
export PYTHONDEVMODE=1

mkdir -p "${CCACHE_DIR}"
mkdir -p "${COV_DIR}"

python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install flake8 pytest pytest-faulthandler coverage

export CFLAGS="-coverage -ftest-coverage -fprofile-arcs -Werror"

# MESON
/usr/bin/python3 -m pip install --user meson
export PATH="${HOME}/.local/bin:${PATH}"
export PKG_CONFIG_PATH="$(python -c 'import sys; sys.stdout.write(sys.prefix)')/lib/pkgconfig"

meson _build -Dpython="$(which python)"
ninja -C _build
xvfb-run -a meson test --suite pygobject --timeout-multiplier 4 -C _build -v
rm -Rf _build

# CODE QUALITY
python -m flake8

# DOCUMENTATION CHECKS
if [[ "${PYVER}" == "2.7" ]] && [[ "${PYIMPL}" == "CPython" ]]; then
    python -m pip install sphinx sphinx_rtd_theme
    python -m sphinx -W -a -E -b html -n docs docs/_build
fi;

# BUILD & TEST AGAIN USING SETUP.PY
python setup.py build_tests
xvfb-run -a python -m coverage run tests/runtests.py

# COLLECT GCOV COVERAGE
lcov --rc lcov_branch_coverage=1 --directory . --capture --output-file \
    "${COV_DIR}/${CI_JOB_NAME}.lcov"
