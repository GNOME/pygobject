CLEANFILES =
check_LTLIBRARIES = libgimarshallingtests.la
test_typelibs = GIMarshallingTests-1.0.typelib

nodist_libgimarshallingtests_la_SOURCES = $(GI_DATADIR)/tests/gimarshallingtests.c $(GI_DATADIR)/tests/gimarshallingtests.h
libgimarshallingtests_la_CFLAGS = $(GLIB_CFLAGS)
libgimarshallingtests_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS) -no-undefined

# This is a hack to make sure a shared library is built
libgimarshallingtests.la: $(libgimarshallingtests_la_OBJECTS) $(libgimarshallingtests_la_DEPENDENCIES)
	$(AM_V_GEN) $(LINK) -rpath $(pkgpyexecdir) $(libgimarshallingtests_la_LDFLAGS) $(libgimarshallingtests_la_OBJECTS) $(libgimarhallingtests_la_LIBADD) $(LIBS)

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

# regress.c needs cairo
if ENABLE_CAIRO
check_LTLIBRARIES += libregress.la
test_typelibs += Regress-1.0.typelib
nodist_libregress_la_SOURCES = $(GI_DATADIR)/tests/regress.c $(GI_DATADIR)/tests/regress.h
libregress_la_CFLAGS = $(GIO_CFLAGS) $(CAIRO_CFLAGS)
libregress_la_LDFLAGS = -module -avoid-version $(GIO_LIBS) $(CAIRO_LIBS) -no-undefined

libregress.la: $(libregress_la_OBJECTS) $(libregress_la_DEPENDENCIES)
	$(AM_V_GEN) $(LINK) -rpath $(pkgpyexecdir) $(libregress_la_LDFLAGS) $(libregress_la_OBJECTS) $(libregress_la_LIBADD) $(LIBS)

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

endif # ENABLE_CAIRO


gschemas.compiled: org.gnome.test.gschema.xml
	glib-compile-schemas --targetdir=. --schema-file=$<

CLEANFILES += Regress-1.0.gir Regress-1.0.typelib GIMarshallingTests-1.0.gir GIMarshallingTests-1.0.typelib gschemas.compiled

check_LTLIBRARIES += testhelper.la

testhelper_la_CFLAGS = -I$(top_srcdir)/gi $(PYTHON_INCLUDES) $(GLIB_CFLAGS)
testhelper_la_LDFLAGS = -module -avoid-version -no-undefined $(PYTHON_LIBS)
testhelper_la_LIBADD = $(GLIB_LIBS)
testhelper_la_SOURCES = \
	testhelpermodule.c \
	test-floating.c \
	test-thread.c \
	test-unknown.c

# This is a hack to make sure a shared library is built
testhelper.la: $(testhelper_la_OBJECTS) $(testhelper_la_DEPENDENCIES)
	$(AM_V_GEN) $(LINK) -rpath $(pkgpyexecdir) $(testhelper_la_LDFLAGS) $(testhelper_la_OBJECTS) $(testhelper_la_LIBADD) $(LIBS)


.la.$(OS_EXT):
	test -L $@ || $(LN_S) .libs/$@ $@


all: $(check_LTLIBRARIES:.la=.$(OS_EXT))

EXTRA_DIST = \
	compathelper.py \
	runtests.py \
	runtests-windows.py \
	testmodule.py \
	test-floating.h \
	test-thread.h \
	test-unknown.h \
	te_ST@nouppera \
	org.gnome.test.gschema.xml \
	test_cairo.py \
	test_error.py \
	test_gio.py \
	test_glib.py \
	test_gobject.py \
	test_gtype.py \
	test_interface.py \
	test_internal_api.py \
	test_iochannel.py \
	test_mainloop.py \
	test_object_marshaling.py \
	test_option.py \
	test_properties.py \
	test_signal.py \
	test_source.py \
	test_subprocess.py \
	test_thread.py \
	test_typeclass.py \
	test_everything.py \
	test_gi.py \
	test_gdbus.py \
	test_import_machinery.py \
	test_overrides_glib.py \
	test_overrides_pango.py \
	test_overrides_gdk.py \
	test_overrides_gtk.py \
	test_atoms.py \
	test_generictreemodel.py \
	test_docstring.py \
	test_repository.py \
	compat_test_pygtk.py \
	gi/__init__.py \
	gi/overrides/__init__.py \
	gi/overrides/Regress.py \
	$(NULL)

clean-local:
	rm -f $(check_LTLIBRARIES:.la=.$(OS_EXT)) file.txt~

DBUS_LAUNCH=$(shell which dbus-launch)
RUN_TESTS_ENV_VARS= \
	PYTHONPATH=$(top_builddir):$(top_builddir)/tests:$${PYTHONPATH:+:$$PYTHONPATH} \
	LD_LIBRARY_PATH=$(builddir)/.libs:$$LD_LIBRARY_PATH \
	GI_TYPELIB_PATH=$(builddir):$$GI_TYPELIB_PATH \
	XDG_DATA_DIRS=$$XDG_DATA_DIRS:/usr/share \
	MALLOC_PERTURB_=85 \
	MALLOC_CHECK_=3 \
	G_SLICE=debug-blocks \
	TESTS_BUILDDIR=$(builddir)

# pygtkcompat tests need to be run in a separate process as they
# clobber global name space
check-local: $(check_LTLIBRARIES:.la=.$(OS_EXT)) $(test_typelibs) gschemas.compiled
	export `$(DBUS_LAUNCH)` && \
	$(RUN_TESTS_ENV_VARS) $(EXEC_NAME) $(PYTHON) -Wd $(srcdir)/runtests.py; rc=$$?; \
	[ "$$rc" -ne 0 ] || [ -n "$$TEST_NAMES" ] || { TEST_NAMES=compat_test_pygtk $(RUN_TESTS_ENV_VARS) $(EXEC_NAME) $(PYTHON) -Wd -Werror::PendingDeprecationWarning -Werror::DeprecationWarning -Werror::RuntimeWarning $(srcdir)/runtests.py; rc=$$?; }; \
	kill $$DBUS_SESSION_BUS_PID; \
	exit $$rc

check.gdb:
	EXEC_NAME="gdb --args" $(MAKE) check

check.nemiver:
	EXEC_NAME="nemiver" $(MAKE) check

# Note G_SLICE in these check variations is intended to clobber the default set in RUN_TESTS_ENV_VARS
check.valgrind:
	EXEC_NAME="G_SLICE=always-malloc G_DEBUG=gc-friendly \
	valgrind --leak-check=full --show-possibly-lost=no --num-callers=20 \
	--suppressions=$(top_srcdir)/tests/$(PYTHON_BASENAME).supp \
	" \
	$(MAKE) check

check.valgrindlog:
	mkdir -p $(top_builddir)/tmp
	EXEC_NAME="G_SLICE=always-malloc G_DEBUG=gc-friendly \
	valgrind --leak-check=full --show-possibly-lost=no --num-callers=20 \
	--suppressions=$(top_srcdir)/tests/$(PYTHON_BASENAME).supp \
	--child-silent-after-fork=yes \
	--log-file=$(top_builddir)/tmp/`git rev-parse HEAD | cut -c1-8`-$$TEST_NAMES.log \
	" \
	$(MAKE) check

check.valgrindxml:
	mkdir -p $(top_builddir)/tmp
	EXEC_NAME="G_SLICE=always-malloc G_DEBUG=gc-friendly \
	valgrind --leak-check=full --show-possibly-lost=no --num-callers=20 \
	--suppressions=$(top_srcdir)/tests/$(PYTHON_BASENAME).supp \
	--child-silent-after-fork=yes \
	--xml=yes --xml-file=$(top_builddir)/tmp/`git rev-parse HEAD | cut -c1-8`-$$TEST_NAMES.xml \
	" \
	$(MAKE) check
