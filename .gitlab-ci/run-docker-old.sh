#!/bin/bash

set -e

TAG="lazka/pygobject:pyenv-old"

sudo docker build --build-arg HOST_USER_ID="$UID" --tag "${TAG}" \
    --file "Dockerfile.old" .
sudo docker run --rm \
    --volume "$(pwd)/..:/home/user/app" --workdir "/home/user/app" \
    --tty --interactive "${TAG}" bash
