#!/bin/bash

set -e

# Make the Windows paths match our current layout
python ./.gitlab-ci/fixup-lcov-paths.py coverage/*.lcov

# Remove external headers (except gi tests)
for path in coverage/*.lcov; do
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '/usr/include/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '/usr/local/include/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '/opt/pypy/include/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '/home/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '*/msys64/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '*site-packages/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '*subprojects/*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '*tmp-introspect*' -o "${path}"
    lcov --config-file .gitlab-ci/lcovrc -r "${path}" '*/meson-private/*' -o "${path}"

    test -s "${path}" || rm "${path}"
done

genhtml --config-file .gitlab-ci/lcovrc \
    coverage/*.lcov -o coverage/

cd coverage
rm -f .coverage*
rm -f ./*.lcov
