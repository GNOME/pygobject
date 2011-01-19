#!/usr/bin/env python
# -*- Mode: Python -*-

import os
import glob
import sys

import unittest

if '--help' in sys.argv:
    print "Usage: ./runtests.py <testfiles>"
    sys.exit(0)

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
runner.run(suite)

