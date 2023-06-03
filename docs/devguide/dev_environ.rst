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
    sudo apt-get install -y libgirepository1.0-dev build-essential \
      libbz2-dev libreadline-dev libssl-dev zlib1g-dev libsqlite3-dev wget \
      curl llvm libncurses5-dev libncursesw5-dev xz-utils tk-dev libcairo2-dev


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

.. code:: console

    pacman -S --needed --noconfirm base-devel mingw-w64-x86_64-toolchain git \
       mingw-w64-x86_64-python3 mingw-w64-x86_64-python3-cairo \
       mingw-w64-x86_64-gobject-introspection mingw-w64-x86_64-libffi

.. _macosx-dep:

|macosx-logo| macOS
-------------------

With homebrew:

.. code:: console

    brew update
    brew install pyenv pipx
    brew install pipx
    pipx ensurepath


.. _install-pyenv:

Install `pyenv`_
================

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

    pyenv install 3.11
    pyenv global 3.11


.. _install-poetry:

Install `Poetry`_
=================

`Poetry`_ is a tool for dependency management and packaging in Python, we'll install it
with `pipx`_ which installs Python CLI tools in to separate virtualenvs.

============================================== ==========================================
|linux-logo| :ref:`Linux <linux-poetry>`       |macosx-logo| :ref:`macOS <macosx-poetry>`
|windows-logo| :ref:`Windows <windows-poetry>`
============================================== ==========================================

.. _linux-poetry:

|linux-logo| Linux
------------------

.. code:: console

    python3 -m pip install --user pipx
    python3 -m pipx ensurepath
    pipx install poetry


.. _windows-poetry:

|windows-logo| Windows
----------------------

.. code:: console

    python.exe -m pip install --user pipx
    python.exe -m pipx ensurepath
    pipx install poetry


.. _macosx-poetry:

|macosx-logo| macOS
-------------------

With homebrew:

.. code:: console

    pipx install poetry


.. _projects-pygobject-dependencies:


*****************
Work on PyGObject
*****************

.. _platform-ind-steps:

Platform Independent Steps
==========================


If you are going to work on developing PyGObject itself, then do the following
additional steps:

.. code:: console

    git clone https://gitlab.gnome.org/GNOME/pygobject.git
    cd pygobject
    poetry install
    poetry shell


.. _pyenv: https://github.com/pyenv/pyenv
.. _pipx: https://pypa.github.io/pipx/
.. _Poetry: https://python-poetry.org
