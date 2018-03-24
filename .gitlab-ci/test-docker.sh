#!/bin/bash

set -e

python --version

# ccache setup
mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
PYVER=$(python -c "import sys; sys.stdout.write(''.join(map(str, sys.version_info[:3])))")
PYIMPL=$(python -c "import sys, platform; sys.stdout.write(platform.python_implementation())")
SOURCE_DIR="$(pwd)"
PY_PREFIX="$(python -c 'import sys; sys.stdout.write(sys.prefix)')"
COV_DIR="${SOURCE_DIR}/coverage"
export PKG_CONFIG_PATH="${PY_PREFIX}/lib/pkgconfig"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
export G_SLICE="debug-blocks"
export COVERAGE_FILE="${COV_DIR}/.coverage.${PYVER}"
export CFLAGS="-coverage -ftest-coverage -fprofile-arcs"

if [[ "${PYIMPL}" == "PyPy" ]]; then
    # https://bitbucket.org/pypy/pypy/issues/2776
    export MALLOC_CHECK_=
    python -m pip install pycairo pytest
else
    python -m pip install git+https://github.com/pygobject/pycairo.git
    python -m pip install flake8 pytest pytest-faulthandler coverage
fi;

if [[ "${PYIMPL}" == "PyPy" ]]; then
    python setup.py build_tests
    exit 0;
fi;

mkdir -p "${COV_DIR}"
rm -Rf /tmp/build
mkdir /tmp/build
cd /tmp/build

# BUILD
"${SOURCE_DIR}"/autogen.sh --with-python=python
make -j8

# TESTS
xvfb-run -a make check

# CODE QUALITY CHECKS
make check.quality

cd "${SOURCE_DIR}"

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
