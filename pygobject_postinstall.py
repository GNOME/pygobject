
"""pygtk is now installed on your machine.

Local configuration files were successfully updated."""

import os, re, sys

prefix_pattern=re.compile("^prefix=.*")

def replace_prefix(s):
    if prefix_pattern.match(s):
        s='prefix='+sys.prefix.replace("\\","\\\\")+'\n'
    return s


if len(sys.argv) == 2 and sys.argv[1] == "-install":

    filenames=['lib/pkgconfig/pygobject-2.0.pc']
    for filename in filenames: 
        pkgconfig_file = os.path.normpath(
            os.path.join(sys.prefix,filename))

        lines=open(pkgconfig_file).readlines()
        open(pkgconfig_file, 'w').writelines(map(replace_prefix,lines))

    print __doc__
