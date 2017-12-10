.. include:: ../icons.rst

======================
Application Deployment
======================

There is currently no nice deployment story, but it's not impossible. This is
a list of random notes and examples.

|linux-logo| Linux
------------------

On Linux there is no single strategy. Quod Libet uses distutils, MyPaint uses
SCons. Gramps uses distutils.

|macosx-logo| macOS
-------------------

On OSX you can use `gtk-osx <https://git.gnome.org/browse/gtk-osx>`__ which is
based on jhbuild and then `gtk-mac-bundler
<https://git.gnome.org/browse/gtk-mac-bundler>`__ for packaging things up and
making libraries relocatable. With macOS bundles you generally have a startup
shell script which sets all the various env vars relative to the bundle,
similar to jhbuild.

|windows-logo| Windows
----------------------

On Windows things are usually build to be relocatable by default, so no env
vars are needed. You can build/install through MSYS2, copy the bits you need
and you are done. For GUI application you'll also need an exe launcher that
links against the python dll.

Example Deployments
-------------------

* `Quod Libet <https://quodlibet.readthedocs.io/>`__ provides a Windows
  installer based on MSYS2 and NSIS3. On macOS, jhbuild is used for building,
  gtk.mac-bundler for packing things up and `dmgbuild
  <https://pypi.python.org/pypi/dmgbuild>`__ for creating a dmg. distutis is
  used for building/installing the application into the final environment.
  Most of this is automated and scripts can be found in the git repo.

* `MyPaint <http://mypaint.org/>`__ provides a Windows installer based on
  MSYS2 and Inno Setup. It uses SCons for building/installing the application.

* ...?

Other options
-------------

* `PyInstaller <http://www.pyinstaller.org/>`_ is a program that freezes (packages) Python programs into stand-alone executables, under Windows, Linux, Mac OS X, and more. PyInstaller's packager has built-in support for automatically including PyGObject dependencies with your application without requiring additional configuration.
