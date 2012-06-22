#!/usr/bin/env python

import unittest
import sys

# py3k has StringIO in a different module
try:
    from StringIO import StringIO
    StringIO  # pyflakes
except ImportError:
    from io import StringIO

# FIXME: we need a way to import the options module from a public module
from gi._glib.option import OptionParser, OptionGroup, OptionValueError, \
    make_option, BadOptionError


class TestOption(unittest.TestCase):
    EXCEPTION_MESSAGE = "This callback fails"

    def setUp(self):
        self.parser = OptionParser("NAMES...",
                                   description="Option unit test")
        self.parser.add_option("-t", "--test", help="Unit test option",
                               action="store_false", dest="test", default=True)
        self.parser.add_option("--g-fatal-warnings",
                               action="store_true",
                               dest="fatal_warnings",
                               help="dummy"),

    def _create_group(self):
        def option_callback(option, opt, value, parser):
            raise Exception(self.EXCEPTION_MESSAGE)

        group = OptionGroup(
            "unittest", "Unit test options", "Show all unittest options",
            option_list=[
                make_option("-f", "-u", "--file", "--unit-file",
                            type="filename",
                            dest="unit_file",
                            help="Unit test option"),
                make_option("--test-integer",
                            type="int",
                            dest="test_integer",
                            help="Unit integer option"),
                make_option("--callback-failure-test",
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
        self.assertFalse(args)

        options, args = self.parser.parse_args(
            ["test_option.py", "foo"])
        self.assertEqual(args, [])

        options, args = self.parser.parse_args(
            ["test_option.py", "foo", "bar"])
        self.assertEqual(args, [])

    def testParseArgsDoubleDash(self):
        options, args = self.parser.parse_args(
            ["test_option.py", "--", "-xxx"])
        #self.assertEqual(args, ["-xxx"])

    def testParseArgsGroup(self):
        group = self._create_group()

        options, args = self.parser.parse_args(
            ["test_option.py", "--test", "-f", "test"])

        self.assertFalse(options.test)
        self.assertEqual(options.unit_file, "test")

        self.assertTrue(group.values.test)
        self.assertFalse(self.parser.values.test)
        self.assertEqual(group.values.unit_file, "test")
        self.assertFalse(args)

    def testOptionValueError(self):
        self._create_group()
        self.assertRaises(OptionValueError, self.parser.parse_args,
                          ["test_option.py", "--test-integer=text"])

    def testBadOptionError(self):
        self.assertRaises(BadOptionError,
                          self.parser.parse_args,
                          ["test_option.py", "--unknwon-option"])

    def testOptionGroupConstructor(self):
        self.assertRaises(TypeError, OptionGroup)

    def testStandardError(self):
        self._create_group()
        sio = StringIO()
        old_stderr = sys.stderr
        sys.stderr = sio
        try:
            self.parser.parse_args(
                ["test_option.py", "--callback-failure-test"])
        finally:
            sys.stderr = old_stderr

        assert (sio.getvalue().split('\n')[-2] ==
                "Exception: " + self.EXCEPTION_MESSAGE)
