#!/usr/bin/env python
import glob
import os
import sys
import unittest

import common

program = None
if len(sys.argv) < 3:
    raise ValueError('Need at least 3 parameters: runtests.py <build-dir> '
                     '<src-dir> <test-module-1> <test-module-2> ...')

buildDir = sys.argv[1]
srcDir = sys.argv[2]
files = sys.argv[3:]

common.importModules(buildDir=buildDir,
                     srcDir=srcDir)

dir = os.path.split(os.path.abspath(__file__))[0]
os.chdir(dir)

def gettestnames():
    names = map(lambda x: x[:-3], files)
    return names

suite = unittest.TestSuite()
loader = unittest.TestLoader()

for name in gettestnames():
    if program and program not in name:
        continue
    suite.addTest(loader.loadTestsFromName(name))

testRunner = unittest.TextTestRunner()
testRunner.run(suite)
