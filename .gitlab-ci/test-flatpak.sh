#!/bin/bash

set -e

export G_DEBUG=""

python3 -m venv _venv
. _venv/bin/activate
python3 -m pip install pytest pytest-faulthandler
python3 setup.py test -s
