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

On OSX you can use `gtk-osx <https://gitlab.gnome.org/GNOME/gtk-osx>`__ which is
based on jhbuild and then `gtk-mac-bundler
<https://gitlab.gnome.org/GNOME/gtk-mac-bundler>`__ for packaging things up and
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
  <https://pypi.org/pypi/dmgbuild>`__ for creating a dmg. distutis is
  used for building/installing the application into the final environment.
  Most of this is automated and scripts can be found in the git repo.

* `MyPaint <https://mypaint.app/>`__ provides a Windows installer based on
  MSYS2 and Inno Setup. It uses SCons for building/installing the application.

* `Passphraser <https://github.com/zevlee/passphraser>`__ uses the Hello World
  GTK template build system (see below).

Other options
-------------

* `PyInstaller <https://www.pyinstaller.org/>`_ is a program that freezes (packages) Python programs into stand-alone executables, under Windows, Linux, Mac OS X, and more. PyInstaller's packager has built-in support for automatically including PyGObject dependencies with your application without requiring additional configuration.
* `Hello World GTK <https://github.com/zevlee/hello-world-gtk>`_ is a template build system for distributing Python-based GTK applications on Windows, macOS, and Linux. First, an application directory is assembled using PyInstaller. Then, a different program is used to package the resulting directory. For Windows, NSIS3 is used. For macOS, the built-in hdiutil is used. For Linux, AppImageKit is used.
