FROM registry.gitlab.gnome.org/gnome/pygobject/main:v11

USER root

RUN python3 -m pip install meson==0.49.2

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y \
    libfribidi-dev \
    libgraphene-1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    libgtk-3-dev \
    libwayland-dev \
    libxml2-dev \
    wayland-protocols \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://gitlab.gnome.org/GNOME/gtk.git \
    && cd gtk \
    && git checkout 833442e1e29e5 \
    &&  meson -Dprefix=/usr -Dbuild-tests=false -Ddemos=false -Dbuild-examples=false -Dprint-backends=none _build \
    && ninja -C _build \
    && ninja -C _build install \
    && cd .. \
    && rm -Rf gtk

USER user
ENV PYENV_VERSION 3.7.3-debug
