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

# Now the sdist for pypi
# Mark odd versions as PEP440 development versions (e.g. 0.1.0 -> 0.1.0.dev0)
if [[ "$VERSION" =~ ^[0-9]\.[0-9]*[13579]\. ]]
then
    meson rewrite kwargs set project / version "${VERSION}".dev0

    git add meson.build
    GIT_AUTHOR_NAME="build-sdist" GIT_AUTHOR_EMAIL=noreply@gnome.org GIT_COMMITTER_NAME="build-sdist" GIT_COMMITTER_EMAIL=noreply@gnome.org git commit -m "Python .dev build commit"
fi

python -m pip install --upgrade build
python -m build --sdist
