#!/bin/bash

set -e

python3 -m pip install --user pytest pytest-faulthandler
python3 setup.py test -s
