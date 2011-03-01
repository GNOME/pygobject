# -*- Mode: Python -*-

import os
import glob
import sys

import unittest


# force untranslated messages, as we check for them in some tests
os.environ['LC_MESSAGES'] = 'C'

# Load tests.
if 'TEST_NAMES' in os.environ:
	names = os.environ['TEST_NAMES'].split()
elif 'TEST_FILES' in os.environ:
	names = []
	for filename in os.environ['TEST_FILES'].split():
		names.append(filename[:-3])
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
	sys.exit(1) # exit code so "make check" reports error

