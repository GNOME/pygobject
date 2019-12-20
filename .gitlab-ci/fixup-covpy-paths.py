import sys
import sqlite3


def main(argv):
    paths = argv[1:]

    for path in paths:
        # https://github.com/nedbat/coveragepy/issues/903
        conn = sqlite3.connect(path)
        conn.execute("UPDATE file set path = REPLACE(path, '\\', '/')")
        conn.commit()
        conn.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
