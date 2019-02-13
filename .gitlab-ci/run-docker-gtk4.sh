#!/bin/bash

set -e

TAG="registry.gitlab.gnome.org/gnome/pygobject/gtk4:v3"

sudo docker build --tag "${TAG}" --file "Dockerfile.gtk4" .
sudo docker run --rm --security-opt label=disable \
    --volume "$(pwd)/..:/home/user/app" --workdir "/home/user/app" \
    --tty --interactive "${TAG}" bash
