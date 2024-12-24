#!/bin/bash

set -e

DIR="$( cd "$( dirname "$0" )" && pwd )"

if [[ "$1" == "inflatpak" ]]; then
    COV_DIR="$(pwd)/coverage"
    COV_KEY="flatpak-$TEST_GTK_VERSION"
    COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
    export COVERAGE_FILE
    mkdir -p "${COV_DIR}"

    python3 --version

    pip install pycairo meson meson-python pytest pytest-cov
    pip install --config-settings=setup-args="-Dtests=true" --no-build-isolation --editable .
    python -m pytest -v --cov
    python3 -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"
    chmod -R 777 "${COV_DIR}"
else
    # https://gitlab.gnome.org/GNOME/gnome-runtime-images/-/issues/7
    DBUS_SYSTEM_BUS_ADDRESS="$(dbus-daemon --session --print-address --fork)"
    export DBUS_SYSTEM_BUS_ADDRESS
    xvfb-run -a flatpak run --user --filesystem=host --share=network --socket=x11 --command=bash org.gnome.Sdk//master -x "${DIR}"/test-flatpak.sh inflatpak
fi
