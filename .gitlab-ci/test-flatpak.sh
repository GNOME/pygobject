#!/bin/bash

set -e

python3 -m pip install --user pytest
python3 setup.py test
