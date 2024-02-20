#!/bin/bash

set -e

rm -Rf _venv_sdist
python3 -m venv _venv_sdist
source _venv_sdist/bin/activate
mkdir -p dist

# First we create a tarball for GNOME
python -m pip install --upgrade meson pycairo
rm -Rf _sdist_build
meson setup _sdist_build
meson dist --no-tests --allow-dirty -C _sdist_build
mv _sdist_build/meson-dist/*.tar.xz dist/

VERSION=$(meson introspect --projectinfo --indent _sdist_build | awk -F'"' '/version/ { print $4 }')

rm -Rf _sdist_build

# Now the sdist for pypi
# Mark odd versions as PEP440 development versions (e.g. 0.1.0 -> 0.1.0.dev0)
meson rewrite kwargs set project / version ${VERSION}.dev0

python -m pip install --upgrade build
python -m build --sdist

meson rewrite kwargs set project / version ${VERSION}
