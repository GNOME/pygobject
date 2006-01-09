
"""pygtk is now installed on your machine.

Local configuration files were successfully updated."""

import os, re, sys

prefix_pattern=re.compile("^prefix=.*")
exec_pattern=re.compile("^exec\s.*")

def replace_prefix(s):
    if prefix_pattern.match(s):
        s='prefix='+sys.prefix.replace("\\","\\\\")+'\n'
    if exec_pattern.match(s):
        s=('exec '+sys.prefix+'\\python.exe '+
           '$codegendir/codegen.py \"$@\"\n').replace("\\","\\\\")
    return s


if len(sys.argv) == 2 and sys.argv[1] == "-install":

    filenames=['lib/pkgconfig/pygtk-2.0.pc','bin/pygtk-codegen-2.0']
    for filename in filenames: 
        pkgconfig_file = os.path.normpath(
            os.path.join(sys.prefix,filename))

        lines=open(pkgconfig_file).readlines()
        open(pkgconfig_file, 'w').writelines(map(replace_prefix,lines))

    print __doc__
