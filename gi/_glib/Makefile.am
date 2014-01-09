AUTOMAKE_OPTIONS = 1.7


pyglibdir = $(pyexecdir)/gi/_glib

pyglib_PYTHON = \
	__init__.py \
	option.py


# if we build in a separate tree, we need to symlink the *.py files from the
# source tree; Python does not accept the extensions and modules in different
# paths
build_pylinks:
	for f in $(pyglib_PYTHON); do \
	    [ -e $(builddir)/$$f ] || $(LN_S) $(srcdir)/$$f $(builddir)/$$f; \
	done


all: build_pylinks
check-local: build_pylinks
