#!/bin/bash

set -e

TAG="registry.gitlab.gnome.org/gnome/pygobject/main:v4"

sudo docker build --build-arg HOST_USER_ID="$UID" --tag "${TAG}" \
    --file "Dockerfile" .
sudo docker run -e PYENV_VERSION='3.6.5' --rm \
    --volume "$(pwd)/..:/home/user/app" --workdir "/home/user/app" \
    --tty --interactive "${TAG}" bash
