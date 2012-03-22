#!/usr/bin/env python
# -*- Mode: Python -*-

import os
import glob
import sys

import unittest

if '--help' in sys.argv:
    print("Usage: ./runtests.py <testfiles>")
    sys.exit(0)

# force untranslated messages, as we check for them in some tests
os.environ['LC_MESSAGES'] = 'C'
os.environ['G_DEBUG'] = 'fatal-warnings fatal-criticals'

# make Gio able to find our gschemas.compiled in tests/. This needs to be set
# before importing Gio. Support a separate build tree, so look in build dir
# first.
os.environ['GSETTINGS_BACKEND'] = 'memory'
os.environ['GSETTINGS_SCHEMA_DIR'] = os.environ.get('TESTS_BUILDDIR',
        os.path.dirname(__file__))

# Load tests.
if 'TEST_NAMES' in os.environ:
    names = os.environ['TEST_NAMES'].split()
elif 'TEST_FILES' in os.environ:
    names = []
    for filename in os.environ['TEST_FILES'].split():
        names.append(filename[:-3])
elif len(sys.argv) > 1:
    names = []
    for filename in sys.argv[1:]:
        names.append(filename.replace('.py', ''))
else:
    names = []
    for filename in glob.iglob("test_*.py"):
        names.append(filename[:-3])

loader = unittest.TestLoader()
suite = loader.loadTestsFromNames(names)


# Run tests.
runner = unittest.TextTestRunner(verbosity=2)
result = runner.run(suite)
if not result.wasSuccessful():
    sys.exit(1)  # exit code so "make check" reports error
