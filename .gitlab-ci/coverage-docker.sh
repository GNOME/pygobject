#!/bin/bash

set -e

python -m pip install coverage

python -m coverage combine _coverage
python -m coverage html -d _coverage/report-python
rm -f _coverage/.coverage*
genhtml --rc lcov_branch_coverage=1 _coverage/*.lcov -o _coverage/report-c
rm -f _coverage/*.lcov
