#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os
import sys
import glob
import unittest

mydir = os.path.dirname(os.path.abspath(__file__))
tests_builddir = os.path.abspath(os.environ.get('TESTS_BUILDDIR', os.path.dirname(__file__)))
builddir = os.path.dirname(tests_builddir)

# we have to do this here instead of Makefile.am so that the implicitly added
# directory for the source file comes after the builddir
sys.path.insert(0, tests_builddir)
sys.path.insert(0, builddir)

os.environ['PYGTK_USE_GIL_STATE_API'] = ''
sys.argv.append('--g-fatal-warnings')

from gi.repository import GObject
GObject.threads_init()


SKIP_FILES = ['runtests',
              'test_mainloop',      # no os.fork on windows
              'test_subprocess']    # blocks on testChildWatch


if __name__ == '__main__':
    testdir = os.path.split(os.path.abspath(__file__))[0]
    os.chdir(testdir)

    def gettestnames():
        files = glob.glob('*.py')
        names = map(lambda x: x[:-3], files)
        map(names.remove, SKIP_FILES)
        return names

    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    for name in gettestnames():
        try:
            suite.addTest(loader.loadTestsFromName(name))
        except Exception as e:
            print('Could not load %s: %s' % (name, e))

    testRunner = unittest.TextTestRunner()
    testRunner.verbosity = 2
    testRunner.run(suite)
