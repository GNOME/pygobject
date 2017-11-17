CLEANFILES =
test_ltlibraries = libgimarshallingtests.la
test_typelibs = GIMarshallingTests-1.0.typelib
extension_ltlibraries =
extension_libadd =

common_ldflags = \
	-avoid-version \
	-rpath $(abs_builddir) \
	-shared

if OS_WIN32
extension_libadd += \
	$(PYTHON_LIBS)

common_ldflags += \
	-no-undefined
endif

# Note for Python extensions we need to use .so on Mac OSX, not .dylib
extension_ldflags = \
	$(common_ldflags) \
	-module \
	-shrext $(PYTHON_SO)

nodist_libgimarshallingtests_la_SOURCES = \
	$(GI_DATADIR)/tests/gimarshallingtests.c \
	$(GI_DATADIR)/tests/gimarshallingtests.h

dist_libgimarshallingtests_la_SOURCES = \
	$(srcdir)/gimarshallingtestsextra.c \
	$(srcdir)/gimarshallingtestsextra.h

# We don't control the source so don't pass WARN_CFLAGS etc
libgimarshallingtests_la_CFLAGS = $(GLIB_CFLAGS)
libgimarshallingtests_la_LDFLAGS = $(common_ldflags)
libgimarshallingtests_la_LIBADD = $(GLIB_LIBS)

GIMarshallingTests-1.0.gir: libgimarshallingtests.la Makefile
	$(AM_V_GEN) $(INTROSPECTION_SCANNER) --include=Gio-2.0 \
	--namespace=GIMarshallingTests --nsversion=1.0 --symbol-prefix=gi_marshalling_tests \
	--warn-all --warn-error \
	--library=libgimarshallingtests.la \
	--libtool="$(top_builddir)/libtool" \
	--output $@ \
	$(nodist_libgimarshallingtests_la_SOURCES) \
	$(dist_libgimarshallingtests_la_SOURCES)

GIMarshallingTests-1.0.typelib: GIMarshallingTests-1.0.gir Makefile
	$(AM_V_GEN) $(INTROSPECTION_COMPILER) $< -o $@

# regress.c needs cairo
test_ltlibraries += libregress.la
test_typelibs += Regress-1.0.typelib
nodist_libregress_la_SOURCES = $(GI_DATADIR)/tests/regress.c $(GI_DATADIR)/tests/regress.h
# We don't control the source so don't pass WARN_CFLAGS etc
if ENABLE_CAIRO
libregress_la_CFLAGS = $(GIO_CFLAGS) $(CAIRO_CFLAGS)
libregress_la_LIBADD = $(GIO_LIBS) $(CAIRO_LIBS)
else
libregress_la_CFLAGS = $(GIO_CFLAGS) -D_GI_DISABLE_CAIRO
libregress_la_LIBADD = $(GIO_LIBS) $(CAIRO_LIBS)
endif
libregress_la_LDFLAGS = $(common_ldflags)

# g-i doesn't ship these as shared libraries anymore; we build them here
Regress-1.0.gir: libregress.la Makefile
	$(AM_V_GEN) $(INTROSPECTION_SCANNER) --include=cairo-1.0 --include=Gio-2.0 \
	--namespace=Regress --nsversion=1.0 \
	--warn-all --warn-error \
	--library=libregress.la \
	--libtool="$(top_builddir)/libtool" \
	--output $@ \
	$(nodist_libregress_la_SOURCES)
Regress-1.0.typelib: Regress-1.0.gir Makefile
	$(AM_V_GEN) $(INTROSPECTION_COMPILER) $< -o $@

gschemas.compiled: org.gnome.test.gschema.xml
	glib-compile-schemas --targetdir=. --schema-file=$<

CLEANFILES += Regress-1.0.gir Regress-1.0.typelib GIMarshallingTests-1.0.gir GIMarshallingTests-1.0.typelib gschemas.compiled

extension_ltlibraries += testhelper.la

testhelper_la_CFLAGS = $(CODE_COVERAGE_CFLAGS) $(WARN_CFLAGS) -I$(top_srcdir)/gi $(PYTHON_INCLUDES) $(GLIB_CFLAGS)
testhelper_la_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
testhelper_la_LDFLAGS = $(WARN_LDFLAGS) $(extension_ldflags)
testhelper_la_LIBADD = $(CODE_COVERAGE_LIBS) $(GLIB_LIBS) $(extension_libadd)

testhelper_la_SOURCES = \
	testhelpermodule.c \
	test-floating.c \
	test-thread.c \
	test-unknown.c

check_LTLIBRARIES = $(test_ltlibraries) $(extension_ltlibraries)

target_libraries = \
	$(test_ltlibraries:.la=.$(OS_EXT)) \
	$(extension_ltlibraries:.la=$(PYTHON_SO))

$(target_libraries):
	test -L $@ || $(LN_S) .libs/$@ $@

EXTRA_DIST = \
	helper.py \
	compathelper.py \
	runtests.py \
	testmodule.py \
	test-floating.h \
	test-thread.h \
	test-unknown.h \
	te_ST@nouppera \
	org.gnome.test.gschema.xml \
	test_cairo.py \
	test_error.py \
	test_fields.py \
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
	test_resulttuple.py \
	test_ossig.py \
	compat_test_pygtk.py \
	gi/__init__.py \
	gi/overrides/__init__.py \
	gi/overrides/Regress.py \
	$(NULL)

clean-local:
	rm -f $(target_libraries) file.txt~

# Unsetting MSYSTEM prevents msys2 from changing os.path.sep to "/"
RUN_TESTS_ENV_VARS= \
	MALLOC_PERTURB_=85 \
	MALLOC_CHECK_=3 \
	G_SLICE=debug-blocks \
	MSYSTEM= \
	TESTS_BUILDDIR=$(builddir)

# if dbus-run-session is available, use it, otherwise disable DBUS
check-local:
	DBUS_ENV=$$(dbus-run-session true && echo "dbus-run-session --" || echo "DBUS_SESSION_BUS_ADDRESS=.") $(MAKE) check.real

# pygtkcompat tests need to be run in a separate process as they
# clobber global name space
check.real: $(target_libraries) $(test_typelibs) gschemas.compiled
	$(RUN_TESTS_ENV_VARS) $(EXTRA_ENV) $(DBUS_ENV) $(EXEC_NAME) $(PYTHON) -Wd $(srcdir)/runtests.py; rc=$$?; \
	[ "$$rc" -ne 0 ] || [ -n "$$TEST_NAMES" ] || { TEST_NAMES=compat_test_pygtk $(RUN_TESTS_ENV_VARS)  $(EXTRA_ENV) $(DBUS_ENV) $(EXEC_NAME) $(PYTHON) -Wd -Werror::PendingDeprecationWarning -Werror::DeprecationWarning -Werror::RuntimeWarning $(srcdir)/runtests.py; rc=$$?; }; \
	exit $$rc

check.gdb:
	EXEC_NAME="gdb --args" $(MAKE) check

check.nemiver:
	EXEC_NAME="nemiver" $(MAKE) check

# Note G_SLICE in these check variations is intended to clobber the default set in RUN_TESTS_ENV_VARS
check.valgrind:
	EXTRA_ENV="G_SLICE=always-malloc G_DEBUG=gc-friendly PYTHONMALLOC=malloc" \
	EXEC_NAME="valgrind --leak-check=full --show-possibly-lost=no --num-callers=20 \
	--suppressions=$(top_srcdir)/tests/$(PYTHON_VALGRIND_SUPP).supp \
	" \
	$(MAKE) check

check.valgrindlog:
	mkdir -p $(top_builddir)/tmp
	EXTRA_ENV="G_SLICE=always-malloc G_DEBUG=gc-friendly PYTHONMALLOC=malloc" \
	EXEC_NAME="valgrind --leak-check=full --show-possibly-lost=no --num-callers=20 \
	--suppressions=$(top_srcdir)/tests/$(PYTHON_VALGRIND_SUPP).supp \
	--child-silent-after-fork=yes \
	--log-file=$(top_builddir)/tmp/`git rev-parse HEAD | cut -c1-8`-$$TEST_NAMES.log \
	" \
	$(MAKE) check

check.valgrindxml:
	mkdir -p $(top_builddir)/tmp
	EXTRA_ENV="G_SLICE=always-malloc G_DEBUG=gc-friendly PYTHONMALLOC=malloc" \
	EXEC_NAME="valgrind --leak-check=full --show-possibly-lost=no --num-callers=20 \
	--suppressions=$(top_srcdir)/tests/$(PYTHON_VALGRIND_SUPP).supp \
	--child-silent-after-fork=yes \
	--xml=yes --xml-file=$(top_builddir)/tmp/`git rev-parse HEAD | cut -c1-8`-$$TEST_NAMES.xml \
	" \
	$(MAKE) check
