# -*- coding: utf-8 -*-

"""pygobject is now installed on your machine.

Local configuration files were successfully updated."""

import os, re, sys

pkgconfig_file = os.path.normpath(
    os.path.join(sys.prefix,
                 'lib/pkgconfig/pygobject-2.0.pc'))

prefix_pattern=re.compile("^prefix=.*")
version_pattern=re.compile("Version: ([0-9]+\.[0-9]+\.[0-9]+)")

def replace_prefix(s):
    if prefix_pattern.match(s):
        s='prefix='+sys.prefix.replace("\\","/")+'\n'
    s=s.replace("@DATADIR@",
                os.path.join(sys.prefix,'share').replace("\\","/"))
    
    return s

if len(sys.argv) == 2:
    if sys.argv[1] == "-install":
        # fixup the pkgconfig file
        lines=open(pkgconfig_file).readlines()
        open(pkgconfig_file, 'w').writelines(map(replace_prefix,lines))
        print __doc__
