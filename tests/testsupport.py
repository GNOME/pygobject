# -*- Mode: Python; py-indent-offset: 4 -*-

import sys
import os
import unittest
import __main__

# Insert parent directory into path
def insert_pygtk_python_paths():
  try:
    filename = __file__
  except:
    filename = sys.argv[0]
  this_dir = os.path.dirname(os.path.abspath(filename))
  parent_dir = os.path.dirname(this_dir)
  sys.path.insert(0, parent_dir)
  
insert_pygtk_python_paths()

def run_all_tests(mods = [__main__]):
  runner = unittest.TextTestRunner()
  for mod in mods:
    for name in dir(mod):
      val = getattr(mod, name)
      try:
        if issubclass(val, unittest.TestCase):
          suite = unittest.makeSuite(val, 'test')
          runner.run(suite)
      except TypeError:
        pass
