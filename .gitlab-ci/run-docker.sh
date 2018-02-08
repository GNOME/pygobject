#!/bin/bash

sudo docker build --build-arg HOST_USER_ID="$UID" --tag "pygobject" \
    --file "Dockerfile" .
sudo docker run -e PYENV_VERSION='3.6.4' --rm \
    --volume "$(pwd)/..:/home/user/app" --workdir "/home/user/app" \
    --tty --interactive "pygobject" bash
