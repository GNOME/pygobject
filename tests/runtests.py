#!/usr/bin/env python
import glob
import os
import sys
import unittest

import common

buildDir = sys.argv[1]
srcDir = sys.argv[2]
common.importModules(buildDir=buildDir,
                     srcDir=srcDir)

SKIP_FILES = ['common', 'runtests']

dir = os.path.split(os.path.abspath(__file__))[0]
os.chdir(dir)

def gettestnames():
    files = glob.glob('*.py')
    names = map(lambda x: x[:-3], files)
    map(names.remove, SKIP_FILES)
    return names
        
suite = unittest.TestSuite()
loader = unittest.TestLoader()

for name in gettestnames():
    suite.addTest(loader.loadTestsFromName(name))
    
testRunner = unittest.TextTestRunner()
testRunner.run(suite)
