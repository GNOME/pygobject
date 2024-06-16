=====================
Debugging & Profiling
=====================

Things can go wrong, these tools may help you find the cause. If you know any
more tricks please share them.


GObject Instance Count Leak Check
---------------------------------

Requires a development (only available in debug mode) version of glib. Jhbuild
recommended.

::

    jhbuild shell
    GOBJECT_DEBUG=instance-count GTK_DEBUG=interactive ./quodlibet.py

* In the GTK Inspector switch to the "Statistics" tab
* Sort by "Cumulative" and do the action which you suspect does leak or where
  you want to make sure it doesn't repeatedly. Like for example opening
  and closing a window or switching between media files to present.
* If something in the "Cumulative" column steadily increases there probably
  is a leak.

cProfile Performance Profiling
------------------------------

* https://docs.python.org/2/library/profile.html
* bundled with python

::

    python -m cProfile -s [sort_order] quodlibet.py > cprof.txt


where ``sort_order`` can one of the following:
calls, cumulative, file, line, module, name, nfl, pcalls, stdname, time

Example output::

             885311 function calls (866204 primitive calls) in 12.110 seconds

       Ordered by: cumulative time

       ncalls  tottime  percall  cumtime  percall filename:lineno(function)
            1    0.002    0.002   12.112   12.112 quodlibet.py:11(<module>)
            1    0.007    0.007   12.026   12.026 quodlibet.py:25(main)
    19392/13067    0.151    0.000    4.342    0.000 __init__.py:639(__get__)
            1    0.003    0.003    4.232    4.232 quodlibetwindow.py:121(__init__)
            1    0.000    0.000    4.029    4.029 quodlibetwindow.py:549(select_browser)
            1    0.002    0.002    4.022    4.022 albums.py:346(__init__)
            ...
            ...

SnakeViz - cProfile Based Visualization
---------------------------------------

* https://jiffyclub.github.io/snakeviz/
* ``pip install snakeviz``

::

    python -m cProfile -o prof.out quodlibet.py
    snakeviz prof.out


Sysprof - System-wide Performance Profiler for Linux
----------------------------------------------------

* https://www.sysprof.com/

::

    sysprof-cli -c "python quodlibet/quodlibet.py"
    sysprof capture.syscap

GDB
---

::

    gdb --args python quodlibet/quodlibet.py
    # type "run" and hit enter


Debugging Wayland Issues
------------------------

::

    mutter --nested --wayland
    # start your app, it should show up in the nested mutter

::

    weston
    # start your app, it should show up in the nested weston


Debugging HiDPI Issue
---------------------

::

    GDK_SCALE=2 ./quodlibet/quodlibet.py

::

    MUTTER_DEBUG_NUM_DUMMY_MONITORS=2 MUTTER_DEBUG_DUMMY_MONITOR_SCALES=1,2 mutter --nested --wayland
    # start your app, it should show up in the nested mutter
