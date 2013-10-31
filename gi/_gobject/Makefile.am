AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 3.0

pygobjectdir = $(pyexecdir)/gi/_gobject

pygobject_PYTHON = \
	__init__.py

# if we build in a separate tree, we need to symlink the *.py files from the
# source tree; Python does not accept the extensions and modules in different
# paths
build_pylinks:
	for f in $(pygobject_PYTHON); do \
	    [ -e $(builddir)/$$f ] || $(LN_S) $(srcdir)/$$f $(builddir)/$$f; \
	done


all: build_pylinks
check-local: build_pylinks
