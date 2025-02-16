#!/bin/bash

set -e

python -m pip install --upgrade pdm
python -m pdm build

VERSION=$(grep version meson.build | head -1 | sed "s/^.*'\([0-9\.]*\)'.*$/\1/")

[[ "$VERSION" =~ ^[0-9]\.[0-9]*[13579]\. ]] && cat << EOF

****************** ATTENTION ******************
          This is an UNstable release.
       Do NOT upload this release to PyPI.
****************** ATTENTION ******************

EOF
