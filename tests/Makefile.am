noinst_PYTHON = \
	runtests.py \
	test_gi.py

check-local:
	LD_LIBRARY_PATH=$(srcdir)/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH} \
	$(PYTHON) $(srcdir)/runtests.py $$TEST_NAMES

