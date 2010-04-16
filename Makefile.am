ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = \
	 -Wall \
	 -g

SUBDIRS = \
	gi \
	tests

check.gdb:
	cd tests && $(MAKE) check.gdb

%.gdb:
	cd tests && $(MAKE) $*.gdb

check.valgrind:
	cd tests && $(MAKE) check.valgrind

%.valgrind:
	cd tests && $(MAKE) $*.valgrind

