import sys
import os
import re


def main(argv):
    # Fix paths in lcov files generated on a Windows host so they match our
    # current source layout.
    paths = argv[1:]

    for path in paths:
        print(f"cov-fixup: {path}")
        with open(path, encoding="utf-8") as h:
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

        text = re.sub(r"SF:(.*?)$", make_abs, text, 0, re.MULTILINE)

        candidate = None
        for old_root in set(re.findall(r":(.*?)/gi/.*?$", text, re.MULTILINE)):
            if candidate is None or len(old_root) < len(candidate):
                candidate = old_root

        if candidate:
            print(f"replacing {candidate} with {new_root}")
            text = text.replace(candidate, new_root)

        with open(path, "w", encoding="utf-8") as h:
            h.write(text)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
