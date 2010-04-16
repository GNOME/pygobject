noinst_PYTHON = \
	runtests.py \
	test_gi.py

check-local:
	LD_LIBRARY_PATH=$(srcdir)/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH} \
	$(EXEC_NAME) $(PYTHON) $(srcdir)/runtests.py $(TEST_NAMES)

check.gdb:
	EXEC_NAME="gdb --args" $(MAKE) check

%.gdb:
	EXEC_NAME="gdb --args" TEST_NAMES=$* $(MAKE) check

check.valgrind:
	EXEC_NAME="valgrind" $(MAKE) check

%.valgrind:
	EXEC_NAME="valgrind" TEST_NAMES=$* $(MAKE) check

