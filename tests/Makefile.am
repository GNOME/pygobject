tests =  \
	conversion.py \
	enum.py \
	gtype.py \
	test_signal.py \
	test_subtype.py

PYTHONPATH = $(top_builddir):$(top_builddir)/gobject:$(top_srcdir):$(top_srcdir)/gtk

check-local: $(top_srcdir)/gtk/__init__.py
	@if ! test -e $(top_builddir)/gtk/__init__.py; then cp $(top_srcdir)/gtk/__init__.py $(top_builddir)/gtk/__init__.py; fi
	@PYTHONPATH=$(PYTHONPATH) DIST_CHECK=1 $(PYTHON) $(srcdir)/runtests.py
	@if test "$(top_builddir)" != "$(top_srcdir)"; then rm -f $(top_builddir)/gtk/__init__.py*; fi
	@rm -fr *.pyc 

EXTRA_DIST = $(tests) common.py runtests.py
