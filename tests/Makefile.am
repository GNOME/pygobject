tests =  \
	conversion.py \
	enum.py \
	signal.py

check-local:
	@PYTHONPATH=$(top_builddir):$(top_builddir)/.libs:$(top_builddir)/gtk/.libs $(PYTHON) $(srcdir)/runtests.py
	@rm -fr *.pyc

EXTRA_DIST = $(tests) runtests.py
