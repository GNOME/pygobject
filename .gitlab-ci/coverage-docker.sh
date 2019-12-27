#!/bin/bash

set -e

python -m pip install coverage

# Make the Windows paths match our current layout
python ./.gitlab-ci/fixup-lcov-paths.py coverage/*.lcov
python ./.gitlab-ci/fixup-covpy-paths.py coverage/.coverage*

# Remove external headers (except gi tests)
for path in coverage/*.lcov; do
    lcov --rc lcov_branch_coverage=1 -r "${path}" '/usr/include/*' -o "${path}"
    lcov --rc lcov_branch_coverage=1 -r "${path}" '/home/*' -o "${path}"
done

python -m coverage combine coverage
python -m coverage html --show-contexts --ignore-errors -d coverage/report-python
genhtml --ignore-errors=source --rc lcov_branch_coverage=1 \
    coverage/*.lcov -o coverage/report-c

cd coverage
rm -f .coverage*
rm -f *.lcov

ln -s report-python/index.html index-python.html
ln -s report-c/index.html index-c.html

cat >index.html <<EOL
<html>
<body>
<ul>
<li><a href="report-c/index.html">C Coverage</a></li>
<li><a href="report-python/index.html">Python Coverage</a></li>
</ul>
</body>
</html>
EOL
