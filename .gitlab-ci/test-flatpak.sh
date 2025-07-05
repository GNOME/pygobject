#!/bin/bash

set -e

# https://gitlab.gnome.org/GNOME/gnome-runtime-images/-/issues/7
DBUS_SYSTEM_BUS_ADDRESS="$(dbus-daemon --session --print-address --fork)"
export DBUS_SYSTEM_BUS_ADDRESS
xvfb-run -a flatpak-builder --user --verbose --disable-rofiles-fuse flatpak_ci org.gnome.PyGObject.Devel.json
