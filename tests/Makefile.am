CLEANFILES =
noinst_LTLIBRARIES = 
if ENABLE_INTROSPECTION
noinst_LTLIBRARIES += libregress.la libgimarshallingtests.la

nodist_libregress_la_SOURCES = $(GI_DATADIR)/tests/regress.c $(GI_DATADIR)/tests/regress.h
libregress_la_CFLAGS = $(GIO_CFLAGS) $(PYCAIRO_CFLAGS)
libregress_la_LDFLAGS = -module -avoid-version $(GIO_LIBS) $(PYCAIRO_LIBS)
nodist_libgimarshallingtests_la_SOURCES = $(GI_DATADIR)/tests/gimarshallingtests.c $(GI_DATADIR)/tests/gimarshallingtests.h
libgimarshallingtests_la_CFLAGS = $(GLIB_CFLAGS)
libgimarshallingtests_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS)

# This is a hack to make sure a shared library is built
libregress.la: $(libregress_la_OBJECTS) $(libregress_la_DEPENDENCIES)
	$(LINK) -rpath $(pkgpyexecdir) $(libregress_la_LDFLAGS) $(libregress_la_OBJECTS) $(libregress_la_LIBADD) $(LIBS)

libgimarshallingtests.la: $(libgimarshallingtests_la_OBJECTS) $(libgimarshallingtests_la_DEPENDENCIES)
	$(LINK) -rpath $(pkgpyexecdir) $(libgimarshallingtests_la_LDFLAGS) $(libgimarshallingtests_la_OBJECTS) $(libgimarhallingtests_la_LIBADD) $(LIBS)

# g-i doesn't ship these as shared libraries anymore; we build them here
Regress-1.0.gir: libregress.la Makefile
	$(AM_V_GEN) g-ir-scanner --include=cairo-1.0 --include=Gio-2.0 \
	--namespace=Regress --nsversion=1.0 \
	--warn-all --warn-error \
	--library=libregress.la \
	--libtool="$(top_builddir)/libtool" \
	--output $@ \
	$(nodist_libregress_la_SOURCES)
Regress-1.0.typelib: Regress-1.0.gir Makefile
	$(AM_V_GEN) g-ir-compiler $< -o $@

GIMarshallingTests-1.0.gir: libgimarshallingtests.la Makefile
	$(AM_V_GEN) g-ir-scanner --include=Gio-2.0 \
	--namespace=GIMarshallingTests --nsversion=1.0 --symbol-prefix=gi_marshalling_tests \
	--warn-all --warn-error \
	--library=libgimarshallingtests.la \
	--libtool="$(top_builddir)/libtool" \
	--output $@ \
	$(nodist_libgimarshallingtests_la_SOURCES)
GIMarshallingTests-1.0.typelib: GIMarshallingTests-1.0.gir Makefile
	$(AM_V_GEN) g-ir-compiler $< -o $@

gschemas.compiled: org.gnome.test.gschema.xml
	glib-compile-schemas --targetdir=. --schema-file=$<

CLEANFILES += Regress-1.0.gir Regress-1.0.typelib GIMarshallingTests-1.0.gir GIMarshallingTests-1.0.typelib gschemas.compiled
endif

noinst_LTLIBRARIES += testhelper.la

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

TEST_FILES_STATIC = \
	test_gobject.py \
	test_interface.py \
	test_mainloop.py \
	test_option.py \
	test_properties.py \
	test_signal.py \
	test_source.py \
	test_subprocess.py \
	test_thread.py \
	test_uris.py

if BUILD_GIO
TEST_FILES_GIO = \
	test_gio.py \
	test_gresolver.py \
	test_gsocket.py \
	test_gicon.py \
	test_gcancellable.py
endif 

if ENABLE_INTROSPECTION
TEST_FILES_GI = \
	test_everything.py \
	test_gi.py \
	test_gdbus.py \
	test_overrides.py
endif

EXTRA_DIST = \
	compathelper.py \
	runtests.py \
	testmodule.py \
	test-floating.h \
	test-thread.h \
	test-unknown.h \
	org.gnome.test.gschema.xml

EXTRA_DIST += $(TEST_FILES_STATIC) $(TEST_FILES_GI) $(TEST_FILES_GIO)

clean-local:
	rm -f $(LTLIBRARIES:.la=.so) file.txt~

DBUS_LAUNCH=$(shell which dbus-launch)
RUN_TESTS_ENV_VARS= \
	PYTHONPATH=$(top_builddir):$(top_builddir)/tests:$${PYTHONPATH:+:$$PYTHONPATH} \
	LD_LIBRARY_PATH=$(builddir)/.libs:$$LD_LIBRARY_PATH \
	GI_TYPELIB_PATH=$(builddir):$$GI_TYPELIB_PATH \
	XDG_DATA_DIRS=$$XDG_DATA_DIRS:/usr/share \
	TESTS_BUILDDIR=$(builddir)
RUN_TESTS_LAUNCH=$(RUN_TESTS_ENV_VARS) $(DBUS_LAUNCH) $(EXEC_NAME) $(PYTHON) $(srcdir)/runtests.py

# run tests in separately to avoid loading static and introspection bindings in the same process
check-local: $(LTLIBRARIES:.la=.so) Regress-1.0.typelib GIMarshallingTests-1.0.typelib gschemas.compiled
	TEST_FILES="$(TEST_FILES_STATIC)" $(RUN_TESTS_LAUNCH)
	TEST_FILES="$(TEST_FILES_GI)" $(RUN_TESTS_LAUNCH)
if BUILD_GIO
	TEST_FILES="$(TEST_FILES_GIO)" $(RUN_TESTS_LAUNCH)
endif 

check.gdb:
	EXEC_NAME="gdb --args" $(MAKE) check

check.valgrind:
	EXEC_NAME="valgrind --suppressions=python.supp" G_SLICE=always-malloc G_DEBUG=gc-friendly $(MAKE) check

-include $(top_srcdir)/git.mk
