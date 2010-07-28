# -*- Mode: Python -*-

import os
import glob

import unittest


# Load tests.
if 'TEST_NAMES' in os.environ:
	names = os.environ['TEST_NAMES'].split()
else:
	names = []
	for filename in glob.iglob("test_*.py"):
		names.append(filename[:-3])

loader = unittest.TestLoader()
suite = loader.loadTestsFromNames(names)


# Run tests.
runner = unittest.TextTestRunner(verbosity=2)
runner.run(suite)

