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
        case $(python -c "import sys; print('.'.join(map(str, sys.version_info[:2])))") in \
        3.12) \
            PYPY_VERSION=3.12-v8.0.1; \
            PYPY_SHA256=9d5c82d39d590dc04677f4721738b21d670570cdc9824c003836bafd38556cda; \
            ;; \
        3.11) \
            PYPY_VERSION=3.11-v8.0.1; \
            PYPY_SHA256=7e5f2801ee87d52fb4de096fb650bca4d5605cd8b5fe278dba5500019fa3739e; \
            ;; \
        *) \
            echo Invalid Python version >&2; \
            exit 1; \
            ;; \
        esac; \
        wget -O pypy.tar.gz "https://downloads.python.org/pypy/pypy$PYPY_VERSION-src.tar.gz"; \
        echo "$PYPY_SHA256 *pypy.tar.gz" | sha256sum -c -; \
        tar --extract --file pypy.tar.gz --directory $(python -c "import sysconfig; print(sysconfig.get_path('stdlib'))") --strip-components=3 pypy$PYPY_VERSION-src/lib-python/3/test; \
        rm pypy.tar.gz; \
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
