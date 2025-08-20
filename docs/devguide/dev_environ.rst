.. include:: ../icons.rst

.. _devenv:

##################################
Creating a Development Environment
##################################

This describes how to setup a development environment for working on a project
that uses PyGObject, or for working on PyGObject itself. Please follow the
instructions on ":ref:`gettingstarted`" first, as they are a pre-requirement.

.. _pipenv-setup:

*****************
Environment Setup
*****************

.. _install-dependencies:

Install Dependencies
====================
In order to compile Python and pip install pygobject, dependencies are need for
your operating system.

=========================================== ======================================== ==============================================
|ubuntu-logo| :ref:`Ubuntu <ubuntu-dep>`    |fedora-logo| :ref:`Fedora <fedora-dep>` |arch-logo| :ref:`Arch Linux <arch-dep>`
|windows-logo| :ref:`Windows <windows-dep>` |macosx-logo| :ref:`macOS <macosx-dep>`  |opensuse-logo| :ref:`openSUSE <opensuse-dep>`
=========================================== ======================================== ==============================================

.. _ubuntu-dep:

|ubuntu-logo| Ubuntu / |debian-logo| Debian
-------------------------------------------

.. code:: console

    sudo apt-get install -y python3-venv python3-wheel python3-dev
    sudo apt-get install -y gobject-introspection libgirepository-2.0-dev \
      gir1.2-girepository-3.0 build-essential libbz2-dev libreadline-dev \
      libssl-dev zlib1g-dev libsqlite3-dev wget curl llvm libncurses-dev \
      xz-utils tk-dev libcairo2-dev


.. _fedora-dep:

|fedora-logo| Fedora
--------------------

.. code:: console

    sudo dnf install -y python3-wheel
    sudo dnf install -y gcc zlib-devel bzip2 bzip2-devel readline-devel \
      sqlite sqlite-devel openssl-devel tk-devel git python3-cairo-devel \
      cairo-gobject-devel gobject-introspection-devel


.. _arch-dep:

|arch-logo| Arch Linux
----------------------

.. code:: console

    sudo pacman -S --noconfirm python-wheel
    sudo pacman -S --noconfirm base-devel openssl zlib git gobject-introspection


.. _opensuse-dep:

|opensuse-logo| openSUSE
------------------------

.. code:: console

    sudo zypper install -y python3-wheel gobject-introspection-devel \
      python3-cairo-devel openssl zlib git
    sudo zypper install --type pattern devel_basis


.. _windows-dep:

|windows-logo| Windows
----------------------

To develop on Windows you need to have `MSYS2 <https://msys2.org>`_ installed.

.. code:: console

    pacman -S --needed --noconfirm base-devel mingw-w64-ucrt-x86_64-toolchain git \
       mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-pycairo \
       mingw-w64-ucrt-x86_64-gobject-introspection mingw-w64-ucrt-x86_64-libffi

.. _macosx-dep:

|macosx-logo| macOS
-------------------

With homebrew:

.. code:: console

    brew update
    brew install python3 gobject-introspection libffi
    export PKG_CONFIG_PATH=$(brew --prefix libffi)/lib/pkgconfig  # use /usr/local/ for older Homebrew installs


.. _install-pyenv:

Install `pyenv`_ (Optional)
===========================

`pyenv`_ lets you easily switch between multiple versions of Python.

============================================= =========================================
|linux-logo| :ref:`Linux <linux-pyenv>`       |macosx-logo| :ref:`macOS <macosx-pyenv>`
============================================= =========================================

.. _linux-pyenv:

|linux-logo| Linux
------------------

.. code:: console

    curl https://pyenv.run | bash
    exec $SHELL
    pyenv install 3.11
    pyenv global 3.11


.. _macosx-pyenv:

|macosx-logo| macOS
-------------------

.. code:: console

    brew install pyenv
    pyenv install 3.11
    pyenv global 3.11


.. _projects-pygobject-dependencies:


*****************
Work on PyGObject
*****************

.. _platform-ind-steps:

Platform Independent Steps
==========================


First, check out the source code:

.. code:: console

    git clone https://gitlab.gnome.org/GNOME/pygobject.git
    cd pygobject

With a local copy of PyGObject, there's three ways to start developing:

1. PDM, a modern Python package and dependency manager
2. Pip, the default Python package installer
3. Meson, use the Meson build system directly


PDM
---

Make sure you have `PDM <https://pdm-project.org>`_ 2.13 or newer installed.

Then set up the project by running:

.. code:: console

    pdm install

You can run the unit tests with:

.. code:: console

    pdm run pytest


Pip
---

It's always a good idea to work from within a Python virtual environment.
PyGObject is built with `Meson <https://mesonbuild.com/>`_.
In order to support
`editable installs <https://meson-python.readthedocs.io/en/latest/how-to-guides/editable-installs.html>`_,
Meson-python, Meson, and Ninja should be installed in the virtual environment.

.. code:: console

    python3 -m venv .venv
    source .venv/bin/activate
    python3 -m pip install meson-python meson ninja pycairo pytest pre-commit

.. note::

   For Python 3.12 and newer, also install ``setuptools``, since distutils is no longer provided in the standard library.

Install PyGObject in your local environment with the ``--no-build-isolation`` to allow for dynamic rebuilds

.. code:: console

   pip install --no-build-isolation --config-settings=setup-args="-Dtests=true" -e '.[dev]'

By default the C libraries are built in "release" mode (no debug symbols).
To compile the C libraries with debug symbols, run

.. code:: console

   pip install --no-build-isolation --config-settings=setup-args="-Dbuildtype=debug" --config-settings=setup-args="-Dtests=true" -e '.[dev]'

Open a Python console:

.. code:: python

   from gi.repository import GObject

Run the unittests:

.. code:: console

   pytest


Meson
-----

It's also possible to run the tests from Meson. Tests are still run with Pytest, so it's important
that Pytest is installed.

.. code:: console

   meson setup _build  # Needed only once
   meson test -C _build


Contributing Changes
====================

First off, thank you for considering contributing to PyGObject.
We really appreciate it!

* Create your own fork of the repository
* Add tests for your changes
* Do the changes in your fork
* If you like the change and think the project could use it:

  * Make sure you are following the
    `GNOME Code of Conduct <https://conduct.gnome.org>`_
  * Be sure you have the pre-commit hook installed with:

    .. code:: console

      pre-commit install

    It will ensure that lint and code formatting tools are run automatically.
  * Commit your changes
  * Create a merge request

.. _pyenv: https://github.com/pyenv/pyenv
