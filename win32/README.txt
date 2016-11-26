Instructions for building PyGObject on Visual Studio
====================================================
Building PyGObject on Windows is now also supported using Visual Studio
versions 2008 through 2015, in both 32-bit and 64-bit (x64) flavors, via NMake
Makefiles and using setuptools/distutils (which, in turn, does the NMake build).

You will need Python 2.7.x or Python 3.1+ to build this package,
in addition to a full GLib and gobject-introspection build, preferably
built with the same compiler that is used here to build PyGObject.
The free-of-charge Express/Community editions of Visual Studio are
supported as well.

Since the official Windows installers for Python are built with
Visual Studio, the following lists the recommended Visual Studio/Visual C++
versions to be used for building PyGObject against the official Python/CPython
installer packages from www.python.org:
-Python 2.7.x/3.1.x/3.2.x: 2008
-Python 3.3.x/3.4.x: 2010
-Python 3.5.x/3.6.x: 2015

Again, it is recommended
that if you built your own Python from source, the compiler used to build
Python should match the compiler that is being used here.

The following are instructions for performing such a build, as there mainly
two options that are needed for the build.  A 'clean' target is provided-it is
recommended that one cleans the build and redo the build if any configuration
option changed, as changes in the options can very well produce incompatible
builds from the previous build, due to Release/Debug settings and Python version
differences, since Python modules on Visual Studio builds will link implicitly
to the Python library (pythonxx.dll/pythonxx_d.dll) that is used/specified
during the build.

Note that the build type and platform type (Win32/x64) of Python, GLib,
GObject-Introspection and libffi (and Cairo/Cairo-GObject if the cairo module
in this package is used) that is used in the builds must correspond to the build
type and platform that the attempted build is targetting.  Note that the
setuptools/distutils build only build the release build type.

The build system will determine whether the cairo module in this package will
be built based on:
-The existence of the py2cairo modulein the Python-2.7.x or pycairo module
 in the Python-3.1+ installation.
-and-
-The existence of the the pycairo.h (for Python 2.7.x) or py3cairo.h (for Python
 3.x) in the include\pycairo sub-directory in your Python installation.

To successfully build the cairo module in this package, you will need both Cairo
and Cairo-GObject.

An 'install' target is also provided to copy the built items in their appropriate
locations under
$(srcroot)\build\lib.$(python_windows_platform)-$(python_version_series),
which is described below, which then can be picked up by setuptools and/or
setting PYTHONPATH to that output directory to test the build.  The 'install'
target is also the one that will be in-turn invoked by running the
setuptools/distutils-based build.

Invoke the build by issuing the command:
-in $(srcroot)/win32:
nmake /f Makefile.vc CFG=[release|debug] PYTHON=<full path to python interpreter>

-or in $(srcroot), release builds only-
$(PYTHON) setup.py build, where $(PYTHON) is the full path to python interpreter,
or just python if it is already in your PATH.

where:

CFG: Required for NMake builds.  Choose from a release or debug build.  Note that 
     all builds generate a .pdb file for each .pyd built--this refers
     to the C/C++ runtime that the build uses.  Note that release builds will
     link to pythonxx.dll and debug builds will link to pythonxx_d.dll implicitly.

PYTHON: Required unless the Python interpretor is in your PATH.  Specifying this
        will override the interpreter that is in your PATH.

PREFIX: Optional for overriding the default prefix, $(srcroot)\vs$(VSVER)\$(PLAT)
        if needed.  GLib, g-i and cairo headers and libraries will be searched
        first in $(PREFIX)\include and $(PREFIX)\lib respectively before searching
        in the directories in %INCLUDE% and %LIB%.  Python headers and libraries
        will be searched in the directories that are deduced based on the interpreter
        used.
