import sys
import os
import io
import re


def main(argv):
    # Fix paths in lcov files generated on a Windows host so they match our
    # current source layout.
    paths = argv[1:]

    for path in paths:
        print("cov-fixup:", path)
        text = io.open(path, "r", encoding="utf-8").read()
        text = text.replace("\\\\", "/")
        new_root = os.getcwd()
        for old_root in set(re.findall(":(.*?)/gi/.*?$", text, re.MULTILINE)):
            if old_root != new_root:
                print("replacing %r with %r" % (old_root, new_root))
            text = text.replace(old_root, new_root)
            with io.open(path, "w", encoding="utf-8") as h:
                h.write(text)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
