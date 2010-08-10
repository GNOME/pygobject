
noinst_LTLIBRARIES = testhelper.la

testhelper_la_CFLAGS = -I$(top_srcdir)/gobject $(PYTHON_INCLUDES) $(GLIB_CFLAGS)
testhelper_la_LDFLAGS = -module -avoid-version
testhelper_la_LIBADD = $(GLIB_LIBS)
testhelper_la_SOURCES = \
	testhelpermodule.c \
	test-floating.c \
	test-thread.c \
	test-unknown.c

# This is a hack to make sure a shared library is built
testhelper.la: $(testhelper_la_OBJECTS) $(testhelper_la_DEPENDENCIES)
	$(LINK) -rpath $(pkgpyexecdir) $(testhelper_la_LDFLAGS) $(testhelper_la_OBJECTS) $(testhelper_la_LIBADD) $(LIBS)


.la.so:
	test -L $@ || $(LN_S) .libs/$@ $@


all: $(LTLIBRARIES:.la=.so)


EXTRA_DIST = \
	runtests.py \
	testmodule.py \
	test-floating.h \
	test-thread.h \
	test-unknown.h \
	\
	test_gcancellable.py \
	test_gicon.py \
	test_gio.py \
	test_gobject.py \
	test_gresolver.py \
	test_gsocket.py \
	test_interface.py \
	test_mainloop.py \
	test_option.py \
	test_properties.py \
	test_signal.py \
	test_source.py \
	test_subprocess.py \
	test_thread.py

if ENABLE_INTROSPECTION
EXTRA_DIST += \
	test_everything.py \
	test_gi.py \
	test_overrides.py
endif


clean-local:
	rm -f $(LTLIBRARIES:.la=.so)


check-local: $(LTLIBRARIES:.la=.so)
	PYTHONPATH=$(top_builddir):$(top_builddir)/tests:$${PYTHONPATH:+:$$PYTHONPATH} $(EXEC_NAME) $(PYTHON) $(srcdir)/runtests.py

check.gdb:
	EXEC_NAME="gdb --args" $(MAKE) check

check.valgrind:
	EXEC_NAME="valgrind" G_SLICE=always-malloc G_DEBUG=gc-friendly $(MAKE) check
