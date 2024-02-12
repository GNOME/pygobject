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
rm -Rf _sdist_build

# Now the sdist for pypi
# Mark odd versions as PEP440 development versions (e.g. 0.1.0 -> 0.1.0.dev0)
cp pyproject.toml pyproject.toml.bak
awk '{
        if (/^version = "/) {
            split($0, a, "[ .\"]+");
            if (a[4] % 2 != 0)
                printf "version = \"%s.%s.%s.dev0\"\n", a[3], a[4], a[5];
            else
                print $0;
        } else {
            print $0;
        }
    }' pyproject.toml > _temp && mv _temp pyproject.toml

python -m pip install --upgrade build
python -m build --sdist
mv pyproject.toml.bak pyproject.toml
