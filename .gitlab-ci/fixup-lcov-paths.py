import sys
import os
import re


def main(argv):
    # Fix paths in lcov files generated on a Windows host so they match our
    # current source layout.
    paths = argv[1:]

    for path in paths:
        print("cov-fixup:", path)
        with open(path, "r", encoding="utf-8") as h:
            text = h.read()

        text = text.replace("\\\\", "/").replace("\\", "/")
        new_root = os.getcwd()

        def make_abs(m):
            p = m.group(1)
            if p.startswith("C:/"):
                p = p.replace("C:/", "/c/")
            if not p.startswith("/"):
                p = os.path.join(new_root, p)
            return "SF:" + p

        text = re.sub("SF:(.*?)$", make_abs, text, 0, re.MULTILINE)

        canidate = None
        for old_root in set(re.findall(":(.*?)/gi/.*?$", text, re.MULTILINE)):
            if canidate is None or len(old_root) < len(canidate):
                canidate = old_root

        if canidate:
            print("replacing %r with %r" % (canidate, new_root))
            text = text.replace(canidate, new_root)

        with open(path, "w", encoding="utf-8") as h:
            h.write(text)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
