#!/bin/bash

set -e

virtualenv --python="${PYTHON}" /tmp/venv
source /tmp/venv/bin/activate

python -m pip install git+https://github.com/pygobject/pycairo.git
python -m pip install flake8

export PKG_CONFIG_PATH=/tmp/venv/lib/pkgconfig
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=$((${RANDOM} % 255 + 1))
PYVER=$(python -c "import sys; sys.stdout.write(str(sys.version_info[0]))")

SOURCE_DIR="$(pwd)"
mkdir /tmp/build
cd /tmp/build

# BUILD
"${SOURCE_DIR}"/autogen.sh --with-python=python
make

# TESTS
xvfb-run -a make check

# CODE QUALITY CHECKS
make check.quality

cd "${SOURCE_DIR}"

# DOCUMENTATION CHECKS
if [[ "${PYVER}" == "2" ]]; then
    python -m pip install sphinx sphinx_rtd_theme
    python -m sphinx -W -a -E -b html -n docs docs/_build
fi;

# BUILD & TEST AGAIN USING SETUP.PY
xvfb-run -a python setup.py distcheck
