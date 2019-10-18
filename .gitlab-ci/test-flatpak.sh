#!/bin/bash

set -e

python3 -m pip install --user pytest pytest-faulthandler
# for some reason pip3 fails the first time now..
# https://gitlab.com/freedesktop-sdk/freedesktop-sdk/issues/776
python3 -m pip install --user pytest pytest-faulthandler
python3 setup.py test -s