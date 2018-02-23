#!/bin/bash

set -e

python -m pip install coverage

# Make the Windows paths match our current layout
python ./.gitlab-ci/fixup-cov-paths.py coverage/.coverage* coverage/*.lcov

# Remove external headers (except gi tests)
for path in coverage/*.lcov; do
    lcov --rc lcov_branch_coverage=1 -r "${path}" '/usr/include/*' -o "${path}"
    lcov --rc lcov_branch_coverage=1 -r "${path}" '/home/*' -o "${path}"
done

python -m coverage combine coverage
python -m coverage html --ignore-errors -d coverage/report-python
genhtml --ignore-errors=source --rc lcov_branch_coverage=1 \
    coverage/*.lcov -o coverage/report-c

cd coverage
rm -f .coverage*
rm -f *.lcov

ln -s report-python/index.html index-python.html
ln -s report-c/index.html index-c.html
