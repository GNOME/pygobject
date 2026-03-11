#!/bin/bash

# Perform regression testing with GStreamer

GSTREAMER_VERSION=1.28

set -e

python --version

python -m venv /tmp/venv
# shellcheck disable=SC1091
source /tmp/venv/bin/activate

pip install meson meson-python pycairo
pip install --no-build-isolation .

sudo apt update
sudo apt install -y flex bison libdrm-dev libgudev-1.0-dev libogg-dev \
    libopus-dev libavfilter-dev libsoup-3.0-dev libflac-dev libmp3lame-dev \
    libvpx-dev libnice-dev libjson-glib-dev

git clone --branch $GSTREAMER_VERSION --depth 1 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd gstreamer

meson setup -Dpython=enabled -Dintrospection=enabled -Dugly=disabled _build
VERBOSE=1 meson test -v -C _build --suite gst-python
