#!/bin/bash

set -e

sed -i "/TEST_GTK_VERSION:/s/'.*'/'${TEST_GTK_VERSION:-3.0}'/" .gitlab-ci/org.gnome.PyGObject.Devel.yaml
xvfb-run -a flatpak-builder --user --keep-build-dirs --verbose --disable-rofiles-fuse flatpak_ci .gitlab-ci/org.gnome.PyGObject.Devel.yaml

mv .flatpak-builder/build/pygobject/coverage .
chmod -R 777 coverage
