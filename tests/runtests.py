#!/usr/bin/env python
# -*- Mode: Python -*-

from __future__ import absolute_import

import os
import sys

import pytest


def main(argv):
    if '--help' in argv:
        print("Usage: ./runtests.py <testfiles>")
        return

    mydir = os.path.dirname(os.path.abspath(__file__))

    verbosity_args = []

    if 'PYGI_TEST_VERBOSE' in os.environ:
        verbosity_args += ['--capture=no']

    if 'TEST_NAMES' in os.environ:
        names = os.environ['TEST_NAMES'].split()
    elif 'TEST_FILES' in os.environ:
        names = []
        for filename in os.environ['TEST_FILES'].split():
            names.append(filename[:-3])
    elif len(argv) > 1:
        names = []
        for filename in argv[1:]:
            names.append(filename.replace('.py', ''))
    else:
        return pytest.main([mydir] + verbosity_args)

    def unittest_to_pytest_name(name):
        parts = name.split(".")
        parts[0] = os.path.join(mydir, parts[0] + ".py")
        return "::".join(parts)

    return pytest.main([unittest_to_pytest_name(n) for n in names] + verbosity_args)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
