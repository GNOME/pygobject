INCLUDES = \
	$(PYTHON_INCLUDES) \
	$(GLIB_CFLAGS) \
	$(PANGO_CFLAGS) \
	$(ATK_CFLAGS) \
	-I$(top_srcdir)/gobject

EXTRA_DIST = $(tests) common.py runtests.py

noinst_LTLIBRARIES = testhelper.la
linked_LIBS = testhelper.la

testhelper_la_LDFLAGS = -module -avoid-version
testhelper_la_SOURCES = testhelpermodule.c

tests =  \
	conversion.py \
	enum.py \
	gtype.py \
	test_signal.py \
	test_subtype.py

# This is a hack to make sure a shared library is built
testhelper.la: $(testhelper_la_OBJECTS) $(testhelper_la_DEPENDENCIES)
	$(LINK) -rpath $(pkgpyexecdir) $(testhelper_la_LDFLAGS) $(testhelper_la_OBJECTS) $(testhelper_la_LIBADD) $(LIBS)

PYTHONPATH = $(top_builddir):$(top_builddir)/gobject:$(top_srcdir):$(top_srcdir)/gtk

check-local: testhelper.la $(top_srcdir)/gtk/__init__.py
	@if ! test -e $(top_builddir)/gtk/__init__.py; then cp $(top_srcdir)/gtk/__init__.py $(top_builddir)/gtk/__init__.py; fi
	@PYTHONPATH=$(PYTHONPATH) DIST_CHECK=1 $(PYTHON) $(srcdir)/runtests.py
	@if test "$(top_builddir)" != "$(top_srcdir)"; then rm -f $(top_builddir)/gtk/__init__.py*; fi
	@rm -fr *.pyc 
