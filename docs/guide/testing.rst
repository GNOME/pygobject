.. include:: ../icons.rst

==================================
Testing and Continuous Integration
==================================

To get automated tests of GTK code running on a headless server use Mutter. It allows running your app on Wayland
display server without real display hardware.

::

    export XDG_RUNTIME_DIR=/tmp
    eval $(dbus-launch --auto-syntax)
    mutter --wayland --no-x11 --sm-disable --headless -- python my_script.py
