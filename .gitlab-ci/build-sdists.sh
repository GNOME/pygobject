#!/bin/bash

set -e

rm -Rf _venv_sdist
python3 -m venv _venv_sdist
# shellcheck disable=SC1091
source _venv_sdist/bin/activate
mkdir -p dist

# First we create a tarball for GNOME
python -m pip install --upgrade meson pycairo
rm -Rf _sdist_build
meson setup _sdist_build
meson dist --no-tests --allow-dirty --include-subprojects -C _sdist_build
cp _sdist_build/meson-dist/*.tar.xz dist/

[ -n "${TARBALL_ARTIFACT_PATH}" ] && cp _sdist_build/meson-dist/*.tar.xz "${TARBALL_ARTIFACT_PATH}"

VERSION=$(meson introspect --projectinfo --indent _sdist_build | python -c 'import json, sys; print(json.load(sys.stdin)["version"])')

rm -Rf _sdist_build

[[ "$VERSION" =~ ^[0-9]\.[0-9]*[13579]\. ]] && cat << EOF

****************** ATTENTION ******************
          This is an UNstable release.
       Do NOT upload this release to PyPI.
****************** ATTENTION ******************

EOF
