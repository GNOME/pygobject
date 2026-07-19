ARG BASE=python:3

FROM docker.io/library/$BASE

ENV DEBIAN_FRONTEND=noninteractive

RUN set -eux; \
    apt-get update && apt-get install -y \
    build-essential \
    ccache \
    curl \
    dbus \
    gir1.2-freedesktop-dev \
    gir1.2-girepository-3.0-dev \
    gir1.2-gtk-3.0 \
    gir1.2-gtk-4.0 \
    git \
    gobject-introspection \
    lcov \
    libbz2-dev \
    libcairo2-dev \
    libffi-dev \
    libgirepository-2.0-dev \
    libglib2.0-dev \
    libgtk-3-0 \
    libgtk-4-1 \
    libreadline-dev \
    libsqlite3-dev \
    libssl-dev \
    liblzma-dev \
    ninja-build \
    python3-pip \
    sudo \
    xauth \
    xvfb \
    && rm -rf /var/lib/apt/lists/*

# Add back test modules, based on https://github.com/docker-library/python/blob/master/3.13/trixie/Dockerfile
RUN set -eux; \
    if python -c "import sys; sys.exit(0 if sys.implementation.name == 'pypy' else 1)"; \
    then \
        PYPY_SHA256=f15c9c41e03f3f7ecc25228c6c67427b8918f21ef2d694215994b1fade20f69b; \
        wget -O pypy.tar.bz2 "https://downloads.python.org/pypy/pypy3.11-v7.3.23-src.tar.bz2"; \
        echo "$PYPY_SHA256 *pypy.tar.bz2" | sha256sum -c -; \
        tar --extract --file pypy.tar.bz2 --directory $(python -c "import sysconfig; print(sysconfig.get_path('stdlib'))") --strip-components=3 pypy3.11-v7.3.23-src/lib-python/3/test; \
        rm pypy.tar.bz2; \
    else \
        wget -O python.tar.xz "https://www.python.org/ftp/python/${PYTHON_VERSION%%[a-z]*}/Python-$PYTHON_VERSION.tar.xz"; \
        echo "$PYTHON_SHA256 *python.tar.xz" | sha256sum -c -; \
        tar --extract --file python.tar.xz --directory $(python -c "import sysconfig; print(sysconfig.get_path('stdlib'))") --strip-components=2 Python-$PYTHON_VERSION/Lib/test; \
        rm python.tar.xz; \
    fi

ARG HOST_USER_ID=5555
ENV HOST_USER_ID=${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -ms /bin/bash user \
    && echo 'user ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER user
WORKDIR /home/user

ENV LANG=C.UTF-8
ENV CI=true

ENV PATH="/usr/lib/ccache:${PATH}"
