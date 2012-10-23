pygioverridesdir = $(pyexecdir)/gi/overrides

pygioverrides_PYTHON = \
	GLib.py \
	Gtk.py \
	Gdk.py \
	GObject.py \
	Gio.py \
	GIMarshallingTests.py \
	Pango.py \
	keysyms.py \
	__init__.py

# if we build in a separate tree, we need to symlink the *.py files from the
# source tree; Python does not accept the extensions and modules in different
# paths
build_pylinks:
	for f in $(pygioverrides_PYTHON); do \
	    [ -e $(builddir)/$$f ] || $(LN_S) $(srcdir)/$$f $(builddir)/$$f; \
	done

all-local: build_pylinks
check-local: build_pylinks
