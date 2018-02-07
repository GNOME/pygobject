==================
Building & Testing
==================

Using Autotools
---------------

.. code:: shell

    # Building for Python 2
    ./autogen.sh --with-python=python2
    make

    # Building for Python 3
    ./autogen.sh --with-python=python3
    make

    # Executing some code after the build
    PYTHONPATH=. python3 foo.py

    # To run the test suite
    make check

    # To test only a specific file/class/function::
    make check TEST_NAMES=test_gi
    make check TEST_NAMES=test_gi.TestUtf8
    make check TEST_NAMES=test_gi.TestUtf8.test_utf8_full_return

    # To execute all the tests in a gdb session
    make check.gdb

    # To executes all the tests in valgrind
    make check.valgrind

    # To execute flake8 tests
    make check.quality


Using Setuptools
----------------

.. code:: shell

    # Build in-tree
    python3 setup.py build_ext --inplace

    # Executing some code after the build
    PYTHONPATH=. python3 foo.py

    # Running tests
    python3 setup.py test

    # To test only a specific file/class/function::
    TEST_NAMES=test_gi python3 python3 setup.py test
    TEST_NAMES=test_gi.TestUtf8 python3 setup.py test
    TEST_NAMES=test_gi.TestUtf8.test_utf8_full_return python3 setup.py test

    # Running flake8 tests
    python3 setup.py quality
