#!/bin/sh

set -e

export PYENV_VERSION
PYENV_VERSION="$(pyenv latest "${PYTHON_VERSION:-3}")"
exec "$@"
