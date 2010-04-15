#! /usr/bin/env python

# codegen.py is runnable as a script and available as module in codegen package
if __name__ == '__main__':
    from libcodegen import codegen
    import sys
    sys.exit(codegen.main(sys.argv))
else:
    from codegen.libcodegen.codegen import *


