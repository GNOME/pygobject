CLEANFILES =

# noinst_ always builds a static library
testlib_LTLIBRARIES = libregress.la libgimarshallingtests.la
testlibdir = $(prefix)/unused
install-testlibLTLIBRARIES: # prevent it from being installed

libregress_la_SOURCES = $(GI_DATADIR)/tests/regress.c $(GI_DATADIR)/tests/regress.h
libregress_la_CFLAGS = $(GIO_CFLAGS) $(PYCAIRO_CFLAGS)
libregress_la_LDFLAGS = -avoid-version $(GIO_LIBS) $(PYCAIRO_LIBS)
libgimarshallingtests_la_SOURCES = $(GI_DATADIR)/tests/gimarshallingtests.c $(GI_DATADIR)/tests/gimarshallingtests.h
libgimarshallingtests_la_CFLAGS = $(GLIB_CFLAGS)
libgimarshallingtests_la_LDFLAGS = -avoid-version $(GLIB_LIBS)

# g-i doesn't ship these as shared libraries anymore; we build them here
Regress-1.0.gir: libregress.la Makefile
	$(AM_V_GEN) g-ir-scanner --include=cairo-1.0 --include=Gio-2.0 \
	--namespace=Regress --nsversion=1.0 \
	--warn-all --warn-error \
	--library=libregress.la \
	--libtool="$(top_builddir)/libtool" \
	--output $@ \
	$(libregress_la_SOURCES)
Regress-1.0.typelib: Regress-1.0.gir Makefile
	$(AM_V_GEN) g-ir-compiler $< -o $@

GIMarshallingTests-1.0.gir: libgimarshallingtests.la Makefile
	$(AM_V_GEN) g-ir-scanner --include=Gio-2.0 \
	--namespace=GIMarshallingTests --nsversion=1.0 --symbol-prefix=gi_marshalling_tests \
	--warn-all --warn-error \
	--library=libgimarshallingtests.la \
	--libtool="$(top_builddir)/libtool" \
	--output $@ \
	$(libgimarshallingtests_la_SOURCES)
GIMarshallingTests-1.0.typelib: GIMarshallingTests-1.0.gir Makefile
	$(AM_V_GEN) g-ir-compiler $< -o $@

CLEANFILES += Regress-1.0.gir Regress-1.0.typelib GIMarshallingTests-1.0.gir GIMarshallingTests-1.0.typelib

noinst_LTLIBRARIES = testhelper.la

testhelper_la_CFLAGS = -I$(top_srcdir)/gobject -I$(top_srcdir)/glib $(PYTHON_INCLUDES) $(GLIB_CFLAGS)
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


check-local: $(LTLIBRARIES:.la=.so) Regress-1.0.typelib GIMarshallingTests-1.0.typelib
	PYTHONPATH=$(top_builddir):$(top_builddir)/tests:$${PYTHONPATH:+:$$PYTHONPATH} LD_LIBRARY_PATH=$(builddir)/.libs:$$LD_LIBRARY_PATH GI_TYPELIB_PATH=$(builddir) $(EXEC_NAME) $(PYTHON) $(srcdir)/runtests.py

check.gdb:
	EXEC_NAME="gdb --args" $(MAKE) check

check.valgrind:
	EXEC_NAME="valgrind" G_SLICE=always-malloc G_DEBUG=gc-friendly $(MAKE) check
