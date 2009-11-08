
noinst_LTLIBRARIES = libtestgi.la

libtestgi_la_CFLAGS = $(GNOME_CFLAGS)
libtestgi_la_LDFLAGS = -avoid-version
libtestgi_la_LIBADD = $(GNOME_LIBS)
libtestgi_la_SOURCES = \
	libtestgi.c \
	libtestgi.h

# Force linking of a shared object.
libtestgi_la_LINK = $(LINK) -rpath $(pkgpyexecdir)

TestGI-0.0.gir: libtestgi.la
	$(AM_V_GEN) \
	$(INTROSPECTION_SCANNER) -v \
		--namespace TestGI \
		--nsversion 0.0 \
		$(GNOME_CFLAGS) \
		--include GObject-2.0 \
		--library libtestgi.la \
		--libtool "$(top_builddir)/libtool" \
		--pkg gobject-2.0 \
		--output $@ \
		$(addprefix $(srcdir)/,$(libtestgi_la_SOURCES))

BUILT_GIRSOURCES = TestGI-0.0.gir

noinst_PYTHON = \
	runtests.py \
	test_gi.py

nodist_noinst_DATA = $(BUILT_GIRSOURCES:.gir=.typelib)

CLEANFILES = \
	$(BUILT_GIRSOURCES) \
	$(BUILT_GIRSOURCES:.gir=.typelib)


check-local:
	GI_TYPELIB_PATH=$(srcdir)$${GI_TYPELIB_PATH:+:$$GI_TYPELIB_PATH} \
	LD_LIBRARY_PATH=$(srcdir)/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH} \
	$(PYTHON) $(srcdir)/runtests.py $$TEST_NAMES

.gir.typelib:
	$(AM_V_GEN) \
	LD_LIBRARY_PATH=$(srcdir)/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH} \
	$(INTROSPECTION_COMPILER) --includedir=$(srcdir) $< -o $(@F)
