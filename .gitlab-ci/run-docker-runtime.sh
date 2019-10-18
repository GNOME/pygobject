#!/bin/bash

set -e

TAG="registry.gitlab.gnome.org/gnome/gnome-runtime-images/gnome:master"

sudo docker pull "${TAG}"
sudo docker run --privileged --rm --security-opt label=disable \
    --volume "$(pwd)/..:/home/user/app" --workdir "/home/user/app" \
    --tty --interactive "${TAG}" xvfb-run -a flatpak run --filesystem=host \
    --share=network --socket=x11 --devel --command=bash org.gnome.Sdk//master
