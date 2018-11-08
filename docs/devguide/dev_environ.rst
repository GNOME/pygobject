.. include:: ../icons.rst

.. _devenv:

##################################
Creating a Development Environment
##################################

This describes how to setup a development environment for working on a project
that uses PyGObject, or for working on PyGObject itself. Please follow the
instructions on ":ref:`gettingstarted`" first, as they are a pre-requirement.

.. _pipenv-setup:

************
Pipenv Setup
************

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

    sudo dnf install -y python3-venv python3-wheel
    sudo dnf install -y gcc zlib-devel bzip2 bzip2-devel readline-devel \
      sqlite sqlite-devel openssl-devel tk-devel git python3-cairo-devel \
      cairo-gobject-devel gobject-introspection-devel


.. _arch-dep:

|arch-logo| Arch Linux
----------------------

.. code:: console

    sudo pacman -S --noconfirm python-virtualenv python-wheel
    sudo pacman -S --noconfirm base-devel openssl zlib git gobject-introspection


.. _opensuse-dep:

|opensuse-logo| openSUSE
------------------------

.. code:: console

    sudo zypper install -y python3-venv python3-wheel gobject-introspection-devel \
      python3-cairo-devel openssl zlib git
    sudo zypper install --type pattern devel_basis


.. _windows-dep:

|windows-logo| Windows
----------------------

.. code:: console

    pacman -S --needed --noconfirm base-devel mingw-w64-i686-toolchain git \
       mingw-w64-i686-python3 mingw-w64-i686-python3-cairo \
       mingw-w64-i686-gobject-introspection mingw-w64-i686-libffi

.. _macosx-dep:

|macosx-logo| macOS
-------------------

No extra dependencies needed.


.. _install-pyenv:

Install `pyenv`_
================

`pyenv`_ lets you easily switch between multiple versions of Python.

============================================= =========================================
|linux-logo| :ref:`Linux <linux-pyenv>`       |macosx-logo| :ref:`macOS <macosx-pyenv>`
|windows-logo| :ref:`Windows <windows-pyenv>`
============================================= =========================================

.. _linux-pyenv:

|linux-logo| Linux
------------------

.. code:: console

    git clone https://github.com/pyenv/pyenv.git ~/.pyenv
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
    echo -e 'if command -v pyenv 1>/dev/null 2>&1; then\n  eval "$(pyenv init -)"\nfi' >> ~/.bashrc
    source ~/.bashrc
    pyenv install 3.6.4


.. _windows-pyenv:

|windows-logo| Windows
----------------------

TODO: currently no way to install `pyenv`_ in Windows. So we'll use a normal
`virtualenv`_ instead.

.. code:: console

    virtualenv --python 3 myvenv
    source myvenv/bin/activate


.. _macosx-pyenv:

|macosx-logo| macOS
-------------------

.. code:: console

    brew install pyenv
    pyenv install 3.6.4


.. _install-pipsi:

Install `pipsi`_
================

`pipsi`_ is a wrapper around virtualenv and pip which installs
scripts provided by python packages into separate virtualenvs to shield them
from your system and each other. We'll use this to install pipenv.

============================================= =========================================
|linux-logo| :ref:`Linux <linux-pipsi>`       |macosx-logo| :ref:`macOS <macosx-pipsi>`
|windows-logo| :ref:`Windows <windows-pipsi>`
============================================= =========================================

.. _linux-pipsi:

|linux-logo| Linux
------------------

.. code:: console

    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
    pipsi install pew
    pipsi install pipenv


.. _windows-pipsi:

|windows-logo| Windows
----------------------

.. code:: console

    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi

Add C:/\Users/\.local/\bin to your path via Control Panel->All Control Panel
Items->System->Advanced System Setttings->Environment Variables

.. code:: console

    pipsi install pew
    pipsi install pipenv


.. _macosx-pipsi:

|macosx-logo| macOS
-------------------

With homebrew:

.. code:: console

    brew install pipenv

With pipsi:

.. code:: console

    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
    pipsi install pew
    pipsi install pipenv


.. _projects-pygobject-dependencies:

************************************
Projects with PyGObject Dependencies
************************************

If you are going to work on a project that has PyGObject as a dependency, then
do the following additional steps:

.. code:: console

    git clone <url/projectname.git>
    cd projectname
    pipenv --python 3
    pipenv install pycairo
    pipenv install pygobject
    pipenv shell


.. _work-on-pygobject:

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
    pipenv --python 3
    pipenv install pytest
    pipenv install flake8
    pipenv install pycairo
    pipenv shell


.. _pyenv: https://github.com/pyenv/pyenv
.. _pipsi: https://github.com/mitsuhiko/pipsi
.. _pipenv: https://github.com/pypa/pipenv
.. _virtualenv: https://www.virtualenv.org
