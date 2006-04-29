#!/usr/bin/env python

import unittest

from common import gobject
from gobject import option

class TestOption(unittest.TestCase):

    def setup_group(self):
        group = option.OptionGroup(
            "unittest", "Unit test options", "Show all unittest options",
            option_list = [
                option.make_option("-f", "-u", "--file", "--unit-file",
                                   type="filename",
                                   dest="unit_file",
                                   help="Unit test option"),
            ])
        group.add_option("-t", "--test", 
                         action="store_false", 
                         dest="test", 
                         default=True,
                         help="Unit test option")
        return group

    def setup_parser(self):
        parser = option.OptionParser("NAMES...", description="Option unit test")
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
