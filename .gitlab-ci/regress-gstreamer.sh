#!/bin/bash

# Perform regression testing with GStreamer

GSTREAMER_VERSION=1.28

set -e

python --version

sudo apt update
sudo apt install -y flex bison libdrm-dev libgudev-1.0-dev libogg-dev \
    libopus-dev libavfilter-dev libsoup-3.0-dev libflac-dev libmp3lame-dev \
    libvpx-dev libnice-dev libjson-glib-dev meson ninja-build


git clone --branch $GSTREAMER_VERSION --depth 1 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd gstreamer

cat > subprojects/pygobject.wrap << EOF
[wrap-git]
directory = pygobject
url = https://gitlab.gnome.org/GNOME/pygobject.git
revision = ${CI_COMMIT_SHA:-main}
depth = 1

[provide]
pygobject-3.0 = pygobject_dep
EOF

echo "========== subprojects/pygobject.wrap =========="
cat subprojects/pygobject.wrap
echo "================================================"

meson setup -Dpython=enabled -Dintrospection=enabled -Dugly=disabled _build
meson test -C _build --suite gst-python
