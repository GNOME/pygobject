================
Maintainer Guide
================

Making a Release
----------------

#. Make sure setup.py has the right version number
#. Update NEWS file
#. Run ``python3 setup.py distcheck``, fix any issues and commit.
#. Commit NEWS as ``"release 3.X.Y"`` and push
#. Tag with: ``git tag -s 3.X.Y -m "release 3.X.Y"``
#. Push tag with: ``git push origin 3.X.Y``
#. In case of a stable release, upload to PyPI:
   ``twine upload dist/PyGObject-3.X.Y.tar.gz``
#. Commit post-release version bump to setup.py
#. Create GNOME tarball ``python3 setup.py sdist_gnome``
#. Upload tarball: ``scp pygobject-3.X.Y.tar.xz user@master.gnome.org:``
#. Install tarball:
   ``ssh user@master.gnome.org 'ftpadmin install pygobject-3.X.Y.tar.xz'``
#. In case the release happens on a stable branch copy the NEWS changes to
   the master branch


Branching
---------

Each cycle after the feature freeze, we create a stable branch so development
can continue in the master branch unaffected by the freezes.

#. Create the branch locally with: ``git checkout -b pygobject-3-2``
#. Push new branch: ``git push origin pygobject-3-2``
#. In master, update setup.py to what will be the next version number
   (3.3.0)
