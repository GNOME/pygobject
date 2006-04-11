INCLUDES = \
	$(PYTHON_INCLUDES) \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir)/gobject

EXTRA_DIST = \
	$(tests) \
	common.py \
	runtests.py \
	testmodule.py \
	test-thread.h \
	test-unknown.h

noinst_LTLIBRARIES = testhelper.la
linked_LIBS = testhelper.la

testhelper_la_LDFLAGS = -module -avoid-version
testhelper_la_LIBADD = $(GLIB_LIBS)
testhelper_la_SOURCES = \
	testhelpermodule.c \
	test-thread.c \
	test-unknown.c 

tests =  \
	test_conversion.py \
	test_enum.py \
	test_gtype.py \
	test_mainloop.py \
	test_properties.py \
	test_signal.py \
	test_subprocess.py \
	test_subtype.py \
	test_unknown.py \
	test_source.py

# This is a hack to make sure a shared library is built
testhelper.la: $(testhelper_la_OBJECTS) $(testhelper_la_DEPENDENCIES)
	$(LINK) -rpath $(pkgpyexecdir) $(testhelper_la_LDFLAGS) $(testhelper_la_OBJECTS) $(testhelper_la_LIBADD) $(LIBS)

check-local: $(top_srcdir)/gobject/__init__.py
	@if test "$(top_builddir)" != "$(top_srcdir)"; then \
	   cp $(top_srcdir)/gobject/__init__.py $(top_builddir)/gobject/__init__.py; \
	fi
	@$(PYTHON) $(srcdir)/runtests.py $(top_builddir) $(top_srcdir)
	@if test "$(top_builddir)" != "$(top_srcdir)"; then \
	   rm -f $(top_builddir)/gobject/__init__.py*; \
	fi
	@rm -fr *.pyc 
