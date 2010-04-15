#!/usr/bin/env python

# scmexpr.py is runnable as a script and available as module in codegen package
if __name__ == '__main__':
    from libcodegen import scmrexpr
    import sys
    sys.exit(scmrexpr.main(sys.argv))
else:
    from codegen.libcodegen.scmexpr import *


