from __future__ import print_function

import sys
import os
import io


def main(argv):
    # Fix paths in coverage files to match our current source layout
    # so that coverage report generators can find the source.
    # Mostly needed for Windows.
    paths = argv[1:]

    for path in paths:
        print("cov-fixup:", path)
        text = io.open(path, "r", encoding="utf-8").read()
        text = text.replace("\\\\", "/")
        end = text.index("/gi/")
        try:
            # coverage.py
            start = text[:end].rindex("\"") + 1
        except ValueError:
            # lcov
            start = text[:end].rindex(":") + 1
        old_root = text[start:end]
        new_root = os.getcwd()
        if old_root != new_root:
            print("replacing %r with %r" % (old_root, new_root))
        text = text.replace(old_root, new_root)
        with io.open(path, "w", encoding="utf-8") as h:
            h.write(text)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
