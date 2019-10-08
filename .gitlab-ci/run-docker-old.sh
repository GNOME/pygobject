#!/bin/bash

set -e

TAG="registry.gitlab.gnome.org/gnome/pygobject/old:v3"

sudo docker build --build-arg HOST_USER_ID="$UID" --tag "${TAG}" \
    --file "Dockerfile.old" .
sudo docker run --rm --security-opt label=disable \
    --volume "$(pwd)/..:/home/user/app" --workdir "/home/user/app" \
    --tty --interactive "${TAG}" bash
