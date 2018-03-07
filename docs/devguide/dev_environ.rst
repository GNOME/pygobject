.. include:: ../icons.rst

.. _devenv:

==================================
Creating a Development Environment
==================================

This describes how to setup a development environment for working on a project
that uses PyGObject, or for working on PyGObject itself. Please follow the
instructions on ":ref:`gettingstarted`" first, as they are a pre-requirement.

.. _pipenv:

************
Pipenv Setup
************

.. _ubuntu:

|ubuntu-logo| Ubuntu / |debian-logo| Debian
-------------------------------------------

.. code:: console

    sudo apt-get install -y python3-venv python3-wheel
    sudo apt-get install -y libgirepository1.0-dev build-essential \
      libbz2-dev libreadline-dev libssl-dev zlib1g-dev libsqlite3-dev wget \
      curl llvm libncurses5-dev libncursesw5-dev xz-utils tk-dev
    git clone https://github.com/pyenv/pyenv.git ~/.pyenv
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
    echo -e 'if command -v pyenv 1>/dev/null 2>&1; then\n  eval "$(pyenv init -)"\nfi' >> ~/.bashrc
    ~/.pyenv/bin/pyenv install 3.6.4
    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    ~/.local/bin/pipsi install pew
    ~/.local/bin/pipsi install pipenv


.. _fedora:

|fedora-logo| Fedora
--------------------

.. code:: console


    sudo dnf install -y python3-venv python3-wheel
    sudo dnf install -y gcc zlib-devel bzip2 bzip2-devel readline-devel \
      sqlite sqlite-devel openssl-devel tk-devel git python3-cairo-devel \
      cairo-gobject-devel gobject-introspection-devel
    git clone https://github.com/pyenv/pyenv.git ~/.pyenv
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
    echo -e 'if command -v pyenv 1>/dev/null 2>&1; then\n  eval "$(pyenv init -)"\nfi' >> ~/.bashrc
    ~/.pyenv/bin/pyenv install 3.6.4
    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    ~/.local/bin/pipsi install pew
    ~/.local/bin/pipsi install pipenv


.. _arch:

|arch-logo| Arch Linux
----------------------

.. code:: console

    sudo pacman -S --noconfirm python-virtualenv python-wheel
    sudo pacman -S --noconfirm base-devel openssl zlib git gobject-introspection
    git clone https://github.com/pyenv/pyenv.git ~/.pyenv
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
    echo -e 'if command -v pyenv 1>/dev/null 2>&1; then\n  eval "$(pyenv init -)"\nfi' >> ~/.bashrc
    ~/.pyenv/bin/pyenv install 3.6.4
    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    ~/.local/bin/pipsi install pew
    ~/.local/bin/pipsi install pipenv


.. _opensuse:

|opensuse-logo| openSUSE
------------------------

.. code:: console

    sudo zypper install -y python3-venv python3-wheel gobject-introspection \
      python3-cairo-devel openssl zlib git
    sudo zypper install --type pattern devel_basis
    git clone https://github.com/pyenv/pyenv.git ~/.pyenv
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
    echo -e 'if command -v pyenv 1>/dev/null 2>&1; then\n  eval "$(pyenv init -)"\nfi' >> ~/.bashrc
    ~/.pyenv/bin/pyenv install 3.6.4
    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    ~/.local/bin/pipsi install pew
    ~/.local/bin/pipsi install pipenv

.. _windows:

|windows-logo| Windows
----------------------

TODO: currently no way to install pyenv in Windows

.. code:: console

    pacman -S --needed --noconfirm base-devel mingw-w64-i686-toolchain git \
       mingw-w64-i686-python3 mingw-w64-i686-python3-cairo \
       mingw-w64-i686-gobject-introspection mingw-w64-i686-libffi
    virtualenv --python 3 myvenv
    source myvenv/bin/activate


|macosx-logo| macOS
-------------------

.. code:: console

    brew install pyenv
    pyenv install 3.6.4
    curl https://raw.githubusercontent.com/mitsuhiko/pipsi/master/get-pipsi.py | python3 - --src=git+https://github.com/mitsuhiko/pipsi.git\#egg=pipsi
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.profile
    ~/.local/bin/pipsi install pew
    ~/.local/bin/pipsi install pipenv


.. _otherprojects:

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


.. _pygobjectwork:

*****************
Work on PyGObject
*****************

If you are going to work on developing PyGObject itself, then do the following
additional steps:

.. code:: console

    git clone https://gitlab.gnome.org/GNOME/pygobject.git
    cd pygobject
    pipenv --python 3
    pipenv install pytest
    pipenv install flake8
    pipenv shell


.. _ubuntu:

|ubuntu-logo| Ubuntu / |debian-logo| Debian
-------------------------------------------

.. code:: console

    sudo apt build-dep pygobject
    sudo apt install autoconf-archive
    ./autogen.sh
    make
    make check


.. _fedora:

|fedora-logo| Fedora
--------------------

    sudo dnf builddep pygobject
    ./autogen.sh
    make
    make check


 .. _arch:

|arch-logo| Arch Linux
----------------------

.. code:: console

    makepkg -s pygobject
    ./autogen.sh
    make
    make check


.. _windows:

|windows-logo| Windows
----------------------
    pacman -S --needed --noconfirm autoconf-archive
    ./autogen.sh
    make
    make check