#!/usr/bin/python
#
# Utility script to generate .pc files for GLib
# for Visual Studio builds, to be used for
# building introspection files

# Author: Fan, Chun-wei
# Date: March 10, 2016

import os
import sys

from replace import replace_multi
from pc_base import BasePCItems

def main(argv):
    base_pc = BasePCItems()
    
    base_pc.setup(argv)
    pkg_replace_items = {'@datadir@': '${prefix}/share/pygobject-3.0',
                         '@datarootdir@': '${prefix}/share',
                         '@LIBFFI_PC@': ''}

    pkg_replace_items.update(base_pc.base_replace_items)

    # Generate ..\pygobject-3.0.pc
    replace_multi(base_pc.top_srcdir + '/pygobject-3.0.pc.in',
                  base_pc.srcdir + '/pygobject-3.0.pc',
                  pkg_replace_items)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
