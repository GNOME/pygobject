#!/bin/bash

# Perform regression testing with GStreamer

GSTREAMER_VERSION=1.28

set -e

python --version

sudo apt update
sudo apt install -y flex bison libdrm-dev libgudev-1.0-dev libogg-dev \
    libopus-dev libavfilter-dev libsoup-3.0-dev libflac-dev libmp3lame-dev \
    libvpx-dev libnice-dev libjson-glib-dev meson ninja-build \
    libgtest-dev libopenh264-dev

meson setup -Dtests=false -Dpycairo=disabled --wipe _gi_build
meson compile -C _gi_build
sudo meson install -C _gi_build

git clone --branch $GSTREAMER_VERSION --depth 1 https://gitlab.freedesktop.org/gstreamer/gstreamer.git

meson setup -Dpython=enabled -Dintrospection=enabled -Dugly=disabled _gst_build gstreamer
meson compile -C _gst_build
GST_PLUGIN_SCANNER=$(pwd)/_gst_build/subprojects/gstreamer/libs/gst/helpers/gst-plugin-scanner VERBOSE=1 meson test -v -C _gst_build --suite gst-python

