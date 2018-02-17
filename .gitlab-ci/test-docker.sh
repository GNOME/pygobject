#!/bin/bash

set -e

python --version

python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install flake8 pytest pytest-faulthandler coverage

SOURCE_DIR="$(pwd)"
PY_PREFIX="$(python -c 'import sys; sys.stdout.write(sys.prefix)')"

export PKG_CONFIG_PATH="${PY_PREFIX}/lib/pkgconfig"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
export COVERAGE_FILE="${SOURCE_DIR}/_coverage/.coverage.${PYVER}"
export CFLAGS="-coverage -ftest-coverage -fprofile-arcs"
PYVER=$(python -c "import sys; sys.stdout.write(str(sys.version_info[0]))")

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
python -m coverage run tests/runtests.py

# COLLECT GCOV COVERAGE
lcov --rc lcov_branch_coverage=1 --directory . --capture --output-file \
    "${SOURCE_DIR}/_coverage/${PYVER}.lcov"
