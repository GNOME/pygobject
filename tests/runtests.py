#!/usr/bin/env python
# -*- Mode: Python -*-

import os
import glob
import sys

import unittest

# this was renamed in Python 3, provide backwards compatible name
if sys.version_info[:2] == (2, 7):
    unittest.TestCase.assertRaisesRegex = unittest.TestCase.assertRaisesRegexp

if '--help' in sys.argv:
    print("Usage: ./runtests.py <testfiles>")
    sys.exit(0)

mydir = os.path.dirname(os.path.abspath(__file__))
tests_builddir = os.path.abspath(os.environ.get('TESTS_BUILDDIR', os.path.dirname(__file__)))
builddir = os.path.dirname(tests_builddir)

# we have to do this here instead of Makefile.am so that the implicitly added
# directory for the source file comes after the builddir
sys.path.insert(0, tests_builddir)
sys.path.insert(0, builddir)

# force untranslated messages, as we check for them in some tests
os.environ['LC_MESSAGES'] = 'C'
os.environ['G_DEBUG'] = 'fatal-warnings fatal-criticals'

# make Gio able to find our gschemas.compiled in tests/. This needs to be set
# before importing Gio. Support a separate build tree, so look in build dir
# first.
os.environ['GSETTINGS_BACKEND'] = 'memory'
os.environ['GSETTINGS_SCHEMA_DIR'] = tests_builddir
os.environ['G_FILENAME_ENCODING'] = 'UTF-8'

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
    for filename in glob.iglob(os.path.join(mydir, 'test_*.py')):
        names.append(os.path.basename(filename)[:-3])

loader = unittest.TestLoader()
suite = loader.loadTestsFromNames(names)


# Run tests.
runner = unittest.TextTestRunner(verbosity=2)
result = runner.run(suite)
if not result.wasSuccessful():
    sys.exit(1)  # exit code so "make check" reports error
