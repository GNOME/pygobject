==================
Building & Testing
==================

To pass extra arguments to pytest you can set "PYTEST_ADDOPTS":

.. code:: shell

    # don't hide stdout
    export PYTEST_ADDOPTS="-s"
    python3 setup.py test


Using Setuptools
----------------

.. code:: shell

    # Build in-tree
    python3 setup.py build_ext --inplace

    # Build in-tree including tests
    python3 setup.py build_tests

    # Executing some code after the build
    PYTHONPATH=. python3 foo.py

    # Running tests
    python3 setup.py test

    # To test only a specific file/class/function::
    TEST_NAMES=test_gi python3 python3 setup.py test
    TEST_NAMES=test_gi.TestUtf8 python3 setup.py test
    TEST_NAMES=test_gi.TestUtf8.test_utf8_full_return python3 setup.py test

    # To display stdout and pytest verbose output:
    PYGI_TEST_VERBOSE=yes python3 setup.py test
    # or:
    python3 setup.py test -s

    # using pytest directly
    py.test-3 tests/test_gi.py

    # Running flake8 tests
    python3 setup.py quality

    # Run under gdb
    python3 setup.py test --gdb

    # Run under valgrind
    python3 setup.py test --valgrind --valgrind-log-file=valgrind.log

    # Create a release tarball for GNOME
    python3 setup.py sdist_gnome
