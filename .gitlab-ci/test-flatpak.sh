#!/bin/bash

set -e

DIR="$( cd "$( dirname "$0" )" && pwd )"

if [[ "$1" == "inflatpak" ]]; then
    COV_DIR="$(pwd)/coverage"
    COV_KEY="flatpak-$TEST_GTK_VERSION"
    export COVERAGE_FILE="${COV_DIR}/.coverage.${COV_KEY}"
    mkdir -p "${COV_DIR}"

    python3 -m venv _venv
    . _venv/bin/activate
    python3 -m pip install pytest pytest-faulthandler coverage
    python3 setup.py build_tests
    python3 -m coverage run --context "${COV_KEY}" tests/runtests.py
    python3 -m coverage lcov -o "${COV_DIR}/${COV_KEY}.py.lcov"
    chmod -R 777 "${COV_DIR}"
else
    # https://gitlab.gnome.org/GNOME/gnome-runtime-images/-/issues/7
    export DBUS_SYSTEM_BUS_ADDRESS="$(dbus-daemon --session --print-address --fork)"
    xvfb-run -a flatpak run --user --filesystem=host --share=network --socket=x11 --command=bash org.gnome.Sdk//master -x "${DIR}"/test-flatpak.sh inflatpak
fi
