INCLUDES = \
	$(PYTHON_INCLUDES) \
	$(GLIB_CFLAGS) \
	$(PANGO_CFLAGS) \
	$(ATK_CFLAGS) \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)/gobject

EXTRA_DIST = $(tests) common.py runtests.py test-thread.h test-unknown.h

noinst_LTLIBRARIES = testhelper.la
linked_LIBS = testhelper.la

testhelper_la_LDFLAGS = -module -avoid-version
testhelper_la_LIBADD = $(GLIB_LIBS)
testhelper_la_SOURCES = \
	testhelpermodule.c \
	test-thread.c \
	test-unknown.c 

tests =  \
	test_subprocess.py \
	conversion.py \
	enum.py \
	gtype.py \
	test_dialog.py \
	test_signal.py \
	test_subtype.py \
	test_unknown.py

# This is a hack to make sure a shared library is built
testhelper.la: $(testhelper_la_OBJECTS) $(testhelper_la_DEPENDENCIES)
	$(LINK) -rpath $(pkgpyexecdir) $(testhelper_la_LDFLAGS) $(testhelper_la_OBJECTS) $(testhelper_la_LIBADD) $(LIBS)

check-local: $(top_srcdir)/gtk/__init__.py
	@if test "$(top_builddir)" != "$(top_srcdir)"; then \
	   cp $(top_srcdir)/gtk/__init__.py $(top_builddir)/gtk/__init__.py; \
	   cp $(top_srcdir)/gtk/keysyms.py $(top_builddir)/gtk/keysyms.py; \
	fi
	@$(PYTHON) $(srcdir)/runtests.py $(top_builddir) $(top_srcdir)
	@if test "$(top_builddir)" != "$(top_srcdir)"; then \
	   rm -f $(top_builddir)/gtk/__init__.py*; \
	   rm -f $(top_builddir)/gtk/keysyms.py*; \
	fi
	@rm -fr *.pyc 
