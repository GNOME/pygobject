==================
Building & Testing
==================

Using Autotools
---------------

Building for Python 2:

::

    ./autogen.sh --with-python=python2
    make

Building for Python 3:

::

    ./autogen.sh --with-python=python3
    make


To run the test suite::

    make check

To test only a specific file/class/function::

    make check TEST_NAMES=test_gi
    make check TEST_NAMES=test_gi.TestUtf8
    make check TEST_NAMES=test_gi.TestUtf8.test_utf8_full_return

To execute all the tests in a gdb session::

    make check.gdb

To executes all the tests in valgrind::

    make check.valgrind

To execute pyflakes and pep8 tests::

    make check.quality


Using Setuptools
----------------

Building and testing in the source directory:

::

    python2 setup.py test
    python3 setup.py test
