FROM ubuntu:artful

RUN apt-get update && apt-get install -y \
    autoconf-archive \
    build-essential \
    ccache \
    curl \
    dbus \
    gir1.2-gtk-3.0 \
    git \
    gobject-introspection \
    lcov \
    libbz2-dev \
    libcairo2-dev \
    libffi-dev \
    libgirepository1.0-dev \
    libglib2.0-dev \
    libgtk-3-0 \
    libreadline-dev \
    libsqlite3-dev \
    libssl-dev \
    libtool \
    locales \
    xauth \
    xvfb \
    && rm -rf /var/lib/apt/lists/*

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -ms /bin/bash user

USER user
WORKDIR /home/user

ENV LANG C.UTF-8
ENV CI true
ENV PYENV_ROOT /home/user/.pyenv
ENV PATH="${PYENV_ROOT}/shims:${PYENV_ROOT}/bin:${PATH}"

RUN curl -L https://raw.githubusercontent.com/pyenv/pyenv-installer/master/bin/pyenv-installer | bash

RUN pyenv install 2.7.14
RUN pyenv install 3.4.7
RUN pyenv install 3.5.5
RUN pyenv install 3.6.4
RUN pyenv install 3.7.0b2

ENV PATH="/usr/lib/ccache:${PATH}"
