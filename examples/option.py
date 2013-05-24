#!/usr/bin/env python

from gi.repository import GLib

group = GLib.option.OptionGroup(
    "example", "OptionGroup Example", "Shows all example options",
    option_list=[GLib.option.make_option("--example",
                                         action="store_true",
                                         dest="example",
                                         help="An example option."),
                ])

parser = GLib.option.OptionParser(
    "NAMES ...", description="A simple gobject.option example.",
    option_list=[GLib.option.make_option("--file", "-f",
                                         type="filename",
                                         action="store",
                                         dest="file",
                                         help="A filename option"),
                 # ...
                ])

parser.add_option_group(group)

parser.parse_args()

print("group: example " + str(group.values.example))
print("parser: file " + str(parser.values.file))
