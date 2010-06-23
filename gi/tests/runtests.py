#!/usr/bin/env python

import sys
import unittest
import glob


loader = unittest.TestLoader()

if len(sys.argv) > 1:
	names = sys.argv[1:]
	suite = loader.loadTestsFromNames(names)
else:
	names = []
	for filename in glob.iglob("test_*.py"):
		names.append(filename[:-3])
	suite = loader.loadTestsFromNames(names)

runner = unittest.TextTestRunner(verbosity=2)
runner.run(suite)

