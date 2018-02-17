#!/bin/bash

set -e

python -m pip install coverage

python -m coverage combine coverage
python -m coverage html -d coverage/report-python
genhtml --ignore-errors=source --rc lcov_branch_coverage=1 \
    coverage/*.lcov -o coverage/report-c

cd coverage
rm -f .coverage*
rm -f *.lcov

ln -s report-python/index.html index-python.html
ln -s report-c/index.html index-c.html
