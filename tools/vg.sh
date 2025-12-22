#!/bin/sh
# Run this script from the project root. E.g.
#
#    $ tools/vg.sh tests/test_gi.py
#

valgrind --leak-check=full \
	--suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
	--suppressions=tools/pygobject.supp \
	--show-leak-kinds=definite \
	--num-callers=20 --log-file=vgdump.txt \
	python -m pytest -s "$@"
