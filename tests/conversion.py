# -*- Mode: Python -*-

import unittest

import gtk

class Tests(unittest.TestCase):
  
  def testUnicharArg(self):
      """ Test unichar values when used as arguments. """

      entry = gtk.Entry()
      for valid_value in ['a', u'b', u'\ufff0', u'\ufff0'.encode()]:
          entry.set_invisible_char(valid_value)
          assert entry.get_invisible_char() == unicode(valid_value)

      for invalid_value in ['12', None, 1, '']:
          try:
              entry.set_invisible_char(invalid_value)
          except:
              pass
          else:
              raise AssertionError('exception not raised on invalid value w/ set_invisible_char: %s' 
                                   % invalid_value)
      
  def testUnicharProperty(self):
      """ Test unichar values when used as properties. """
      
      entry = gtk.Entry()
      for valid_value in ['a', u'b', u'\ufff0', u'\ufff0'.encode()]:
          entry.set_property('invisible_char', valid_value)
          assert entry.get_property('invisible_char') == valid_value

      for invalid_value in ['12', None, 1, '']:
          try:
              entry.set_property('invisible_char', invalid_value)
          except:
              pass
          else:
              raise AssertionError('exception not raised on invalid value w/ set_property: %s' 
                                   % invalid_value)
if __name__ == '__main__':
    unittest.main()
