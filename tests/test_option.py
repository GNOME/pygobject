#!/usr/bin/env python

from __future__ import absolute_import

import unittest

# py3k has StringIO in a different module
try:
    from StringIO import StringIO
    StringIO  # pyflakes
except ImportError:
    from io import StringIO

from gi.repository import GLib

from .helper import capture_exceptions


class TestOption(unittest.TestCase):

    def setUp(self):
        self.parser = GLib.option.OptionParser("NAMES...",
                                               description="Option unit test")
        self.parser.add_option("-t", "--test", help="Unit test option",
                               action="store_false", dest="test", default=True)
        self.parser.add_option("--g-fatal-warnings",
                               action="store_true",
                               dest="fatal_warnings",
                               help="dummy"),

    def _create_group(self):
        def option_callback(option, opt, value, parser):
            raise Exception("foo")

        group = GLib.option.OptionGroup(
            "unittest", "Unit test options", "Show all unittest options",
            option_list=[
                GLib.option.make_option("-f", "-u", "--file", "--unit-file",
                                        type="filename",
                                        dest="unit_file",
                                        help="Unit test option"),
                GLib.option.make_option("--test-integer",
                                        type="int",
                                        dest="test_integer",
                                        help="Unit integer option"),
                GLib.option.make_option("--callback-failure-test",
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

    def test_integer(self):
        self._create_group()
        options, args = self.parser.parse_args(
            ["--test-integer", "42", "bla"])
        assert options.test_integer == 42
        assert args == ["bla"]

    def test_file(self):
        self._create_group()

        options, args = self.parser.parse_args(
            ["--file", "fn", "bla"])
        assert options.unit_file == "fn"
        assert args == ["bla"]

    def test_mixed(self):
        self._create_group()

        options, args = self.parser.parse_args(
            ["--file", "fn", "--test-integer", "12", "--test",
             "--g-fatal-warnings", "nope"])

        assert options.unit_file == "fn"
        assert options.test_integer == 12
        assert options.test is False
        assert options.fatal_warnings is True
        assert args == ["nope"]

    def test_parse_args(self):
        options, args = self.parser.parse_args([])
        self.assertFalse(args)

        options, args = self.parser.parse_args(["foo"])
        self.assertEqual(args, ["foo"])

        options, args = self.parser.parse_args(["foo", "bar"])
        self.assertEqual(args, ["foo", "bar"])

    def test_parse_args_double_dash(self):
        options, args = self.parser.parse_args(["--", "-xxx"])
        self.assertEqual(args, ["--", "-xxx"])

    def test_parse_args_group(self):
        group = self._create_group()

        options, args = self.parser.parse_args(
            ["--test", "-f", "test"])

        self.assertFalse(options.test)
        self.assertEqual(options.unit_file, "test")

        self.assertTrue(group.values.test)
        self.assertFalse(self.parser.values.test)
        self.assertEqual(group.values.unit_file, "test")
        self.assertFalse(args)

    def test_option_value_error(self):
        self._create_group()
        self.assertRaises(GLib.option.OptionValueError, self.parser.parse_args,
                          ["--test-integer=text"])

    def test_bad_option_error(self):
        self.assertRaises(GLib.option.BadOptionError,
                          self.parser.parse_args,
                          ["--unknwon-option"])

    def test_option_group_constructor(self):
        self.assertRaises(TypeError, GLib.option.OptionGroup)

    def test_standard_error(self):
        self._create_group()

        with capture_exceptions() as exc:
            self.parser.parse_args(["--callback-failure-test"])

        assert len(exc) == 1
        assert exc[0].value.args[0] == "foo"
