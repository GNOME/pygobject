#!/usr/bin/env python

import unittest
import sys

#from StringIO import StringIO
from common import gobject
from gobject import option

class TestOption(unittest.TestCase):
    EXCEPTION_MESSAGE = "This callback fails"

    def setup_group(self):
        def option_callback(option, opt, value, parser):
            raise StandardError(self.EXCEPTION_MESSAGE)

        group = option.OptionGroup(
            "unittest", "Unit test options", "Show all unittest options",
            option_list = [
                option.make_option("-f", "-u", "--file", "--unit-file",
                                   type="filename",
                                   dest="unit_file",
                                   help="Unit test option"),
                option.make_option("--test-integer",
                                   type="int",
                                   dest="test_integer",
                                   help="Unit integer option"),
                option.make_option("--callback-failure-test",
                                   action="callback",
                                   callback=option_callback,
                                   dest="test_integer",
                                   help="Unit integer option"),
            ])
        group.add_option("-t", "--test", 
                         action="store_false", 
                         dest="test", 
                         default=True,
                         help="Unit test option")
        return group

    def setup_parser(self):
        parser = option.OptionParser("NAMES...", 
                                     description="Option unit test")
        parser.add_option("-t", "--test", help="Unit test option",
                          action="store_false", dest="test", default=True)
        return parser

    def testOption(self):
        parser = self.setup_parser()
        group = self.setup_group()
        parser.add_option_group(group)

        parser.parse_args(["test_option.py", "--test", "-f", "test"])
        assert group.values.test
        assert not parser.values.test
        assert group.values.unit_file == "test"
        
        try:
            parser.parse_args(["test_option.py", "--test-integer=text"])
        except option.OptionValueError:
            pass
        else:
            assert False

        sio = StringIO()
        old_stderr = sys.stderr
        sys.stderr = sio
        try:
            parser.parse_args(["test_option.py", "--callback-failure-test"])
        finally:
            sys.stderr = old_stderr
        assert (sio.getvalue().split('\n')[-2] == 
                "StandardError: " + self.EXCEPTION_MESSAGE)

        try:
            parser.parse_args(["test_option.py", "--unknwon-option"])
        except option.BadOptionError:
            pass
        else:
            assert False

    def testBadConstructor(self):
        self.assertRaises(TypeError, option.OptionGroup)

