#!/usr/bin/env python
# -*- Mode: Python; py-indent-offset: 4 -*-
#
# This litte script outputs the C doc comments to an XML format.
# So far it's only used by gtkmm (The C++ bindings). Murray Cumming.
# Usage example:
# # ./docextract_to_xml.py -s /gnome/head/cvs/gtk+/gtk/ -s /gnome/head/cvs/gtk+/docs/reference/gtk/tmpl/ > gtk_docs.xml

import getopt
import re
import string
import sys

import docextract

def escape_text(unescaped_text):
    # Escape every "&" not part of an entity reference
    escaped_text = re.sub(r'&(?![A-Za-z]+;)', '&amp;', unescaped_text)

    # These weird entities turn up in the output...
    escaped_text = string.replace(escaped_text, '&mdash;', '&#8212;')
    escaped_text = string.replace(escaped_text, '&ast;', '*')
    escaped_text = string.replace(escaped_text, '&percnt;', '%')
    escaped_text = string.replace(escaped_text, '&commat;', '@')

    # Escape for both tag contents and attribute values
    escaped_text = string.replace(escaped_text, '<', '&lt;')
    escaped_text = string.replace(escaped_text, '>', '&gt;')
    escaped_text = string.replace(escaped_text, '"', '&quot;')

    return escaped_text

if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "d:s:o:",
                                   ["source-dir="])
    except getopt.error, e:
        sys.stderr.write('docgen.py: %s\n' % e)
        sys.stderr.write(
            'usage: docgen.py [-s /src/dir]\n')
        sys.exit(1)
    source_dirs = []
    for opt, arg in opts:
        if opt in ('-s', '--source-dir'):
            source_dirs.append(arg)
    if len(args) != 0:
        sys.stderr.write(
            'usage: docgen.py  [-s /src/dir]\n')
        sys.exit(1)

    docs = docextract.extract(source_dirs);
    docextract.extract_tmpl(source_dirs, docs); #Try the tmpl sgml files too.

    # print d.docs

    if docs:

        print "<root>"

        for name, value in docs.items():
            print "<function name=\"" + escape_text(name) + "\">"

            print "<description>"
            #The value is a docextract.FunctionDoc
            print escape_text(value.description)
            print "</description>"

             # Loop through the parameters:
            print "<parameters>"
            for name, description in value.params:
                print "<parameter name=\"" + escape_text(name) + "\">"
                print "<parameter_description>" + escape_text(description) + "</parameter_description>"
                print "</parameter>"

            print "</parameters>"

            # Show the return-type:
            print "<return>" + escape_text(value.ret) + "</return>"

            print "</function>\n"

        print "</root>"
