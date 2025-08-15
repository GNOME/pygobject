===================
Profiling PyGObject
===================

Setup
=====

General vpath build setup with jhbuild shell (x86_64):

.. code-block:: bash

    mkdir _build
    cd _build
    export PYTHON=`which python3.3`
    jhbuild shell
    ../configure --prefix=$JHBUILD_PREFIX --libdir=$JHBUILD_LIBDIR --with-python=$PYTHON
    make

Devise a simple script to profile (myproftest.py):

.. code-block:: python

    from gi.repository import Gtk

    model = Gtk.ListStore(str, str, str, int, int, int)
    columns = [0, 1, 2, 3, 4, 5]
    row = ['a'*16, 'b'*16, 'c'*16, 1, 2, 3]

    for i in range(100000):
        model.insert_with_valuesv(-1, columns, row)

Python Profile
==============

Use cProfile and kcachegrind (via pyprof2calltree):

.. code-block:: bash

    PROFTEST=myproftest
    $PYTHON -m cProfile -o $PROFTEST.pyprof $PROFTEST.py
    pyprof2calltree -i $PROFTEST.pyprof -k

C Extension (callgrind)
=======================

.. code-block:: bash

    PROFTEST=myproftest
    valgrind --tool=callgrind --callgrind-out-file=$PROFTEST.callgrind $PYTHON $PROFTEST.py
    kcachegrind $PROFTEST.callgrind

C Extension (gperftools)
========================

Google PerfTools https://code.google.com/p/gperftools

.. code-block:: bash

    PROFTEST=myproftest
    $PYTHON -m yep -o $PROFTEST.py.prof $PROFTEST.py
    google-pprof --callgrind $PYTHON $PROFTEST.py.prof > $PROFTEST.callgrind
    kcachegrind $PROFTEST.callgrind
