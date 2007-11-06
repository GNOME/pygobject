#!/usr/bin/env python

import unittest
import sys
from StringIO import StringIO

from gobject import option

from common import gobject


class TestOption(unittest.TestCase):
    EXCEPTION_MESSAGE = "This callback fails"

    def setUp(self):
        self.parser = option.OptionParser("NAMES...",
                                     description="Option unit test")
        self.parser.add_option("-t", "--test", help="Unit test option",
                          action="store_false", dest="test", default=True)

    def _create_group(self):
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
        self.parser.add_option_group(group)
        return group

    def testParseArgs(self):
        options, args = self.parser.parse_args(
            ["test_option.py"])
        self.failIf(args)

        options, args = self.parser.parse_args(
            ["test_option.py", "foo"])
        self.assertEquals(args, ["foo"])

        options, args = self.parser.parse_args(
            ["test_option.py", "foo", "bar"])
        self.assertEquals(args, ["foo", "bar"])

    def testParseArgsDoubleDash(self):
        options, args = self.parser.parse_args(
            ["test_option.py", "--", "-xxx"])
        #self.assertEquals(args, ["-xxx"])

    def testParseArgsGroup(self):
        group = self._create_group()

        options, args = self.parser.parse_args(
            ["test_option.py", "--test", "-f", "test"])

        self.failIf(options.test)
        self.assertEqual(options.unit_file, "test")

        self.failUnless(group.values.test)
        self.failIf(self.parser.values.test)
        self.assertEqual(group.values.unit_file, "test")
        self.failIf(args)

    def testOptionValueError(self):
        self.assertRaises(option.OptionValueError, self.parser.parse_args,
                          ["test_option.py", "--test-integer=text"])

    def testBadOptionError(self):
        self.assertRaises(option.BadOptionError,
                          self.parser.parse_args,
                          ["test_option.py", "--unknwon-option"])

    def testOptionGroupConstructor(self):
        self.assertRaises(TypeError, option.OptionGroup)

    def testStandardError(self):
        sio = StringIO()
        old_stderr = sys.stderr
        sys.stderr = sio
        try:
            self.parser.parse_args(
                ["test_option.py", "--callback-failure-test"])
        finally:
            sys.stderr = old_stderr
        assert (sio.getvalue().split('\n')[-2] ==
                "StandardError: " + self.EXCEPTION_MESSAGE)

