.. include:: icons.rst

.. _gettingstarted:

===============
Getting Started
===============

To get things started we will try to run a very simple `GTK+
<https://www.gtk.org/>`_ based GUI application using the :doc:`PyGObject <index>` provided
Python bindings. First create a small Python script called ``hello.py`` with
the following content and save it somewhere:

.. code:: python

    import gi
    gi.require_version("Gtk", "3.0")
    from gi.repository import Gtk

    window = Gtk.Window(title="Hello World")
    window.show()
    window.connect("destroy", Gtk.main_quit)
    Gtk.main()

Before we can run the example application we need to install PyGObject, GTK+
and their dependencies. Follow the instructions for your platform below.

======================================= ==================================== ==================================== ==========================================
|ubuntu-logo| :ref:`Ubuntu <ubuntu>`    |fedora-logo| :ref:`Fedora <fedora>` |arch-logo| :ref:`Arch Linux <arch>` |opensuse-logo| :ref:`openSUSE <opensuse>`
|windows-logo| :ref:`Windows <windows>` |macosx-logo| :ref:`macOS <macosx>`  |python-logo| :ref:`PyPI <pypi>`
======================================= ==================================== ==================================== ==========================================


.. _windows:

|windows-logo| Windows
----------------------

1) Go to http://www.msys2.org/ and download the x86_64 installer
2) Follow the instructions on the page for setting up the basic environment
3) Run ``C:\msys64\mingw32.exe`` - a terminal window should pop up
4) Execute ``pacman -S mingw-w64-i686-gtk3 mingw-w64-i686-python2-gobject mingw-w64-i686-python3-gobject``
5) To test that GTK+3 is working you can run ``gtk3-demo``
6) Copy the ``hello.py`` script you created to ``C:\msys64\home\<username>``
7) In the mingw32 terminal execute ``python3 hello.py`` - a window should appear.

.. figure:: images/start_windows.png
    :scale: 60%


.. _ubuntu:

|ubuntu-logo| Ubuntu / |debian-logo| Debian
-------------------------------------------

1) Open a terminal
2) Execute ``sudo apt install python-gi python-gi-cairo python3-gi python3-gi-cairo gir1.2-gtk-3.0``
3) Change the directory to where your ``hello.py`` script can be found (e.g. ``cd Desktop``)
4) Run ``python3 hello.py``

.. figure:: images/start_linux.png
    :scale: 60%


.. _fedora:

|fedora-logo| Fedora
--------------------

1) Open a terminal
2) Execute ``sudo dnf install pygobject3 python3-gobject gtk3``
3) Change the directory to where your ``hello.py`` script can be found (e.g. ``cd Desktop``)
4) Run ``python3 hello.py``


.. _arch:

|arch-logo| Arch Linux
----------------------

1) Open a terminal
2) Execute ``sudo pacman -S python-gobject python2-gobject gtk3``
3) Change the directory to where your ``hello.py`` script can be found (e.g. ``cd Desktop``)
4) Run ``python3 hello.py``


.. _opensuse:

|opensuse-logo| openSUSE
------------------------

1) Open a terminal
2) Execute ``sudo zypper install python-gobject python3-gobject gtk3``
3) Change the directory to where your ``hello.py`` script can be found (e.g. ``cd Desktop``)
4) Run ``python3 hello.py``


.. _macosx:

|macosx-logo| macOS
-------------------

1) Go to https://brew.sh/ and install homebrew
2) Open a terminal
3) Execute ``brew install pygobject3 --with-python3 gtk+3`` to install for both python2 and python3
4) Change the directory to where your ``hello.py`` script can be found (e.g. ``cd Desktop``)
5) Run ``python3 hello.py``

.. figure:: images/start_macos.png
    :scale: 70%


.. _pypi:

|python-logo| From PyPI
-----------------------

PyGObject is also available on PyPI: https://pypi.org/project/PyGObject

For this approach you have to make sure that all runtime and build
dependencies are present yourself as pip will only take care of pycairo.

.. code::

    virtualenv --python=python3 myvenv
    source myvenv/bin/activate
    pip install pygobject
    python hello.py
