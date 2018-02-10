#!/bin/bash

set -e

python --version

python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install flake8 pytest

PY_PREFIX="$(python -c 'import sys; sys.stdout.write(sys.prefix)')"
export PKG_CONFIG_PATH="${PY_PREFIX}/lib/pkgconfig"
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
PYVER=$(python -c "import sys; sys.stdout.write(str(sys.version_info[0]))")

SOURCE_DIR="$(pwd)"
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
xvfb-run -a python setup.py distcheck
