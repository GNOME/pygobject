================
Maintainer Guide
================

Making a Release
----------------

#. Make sure configure.ac has the right version number
#. Update NEWS file (use ``make release-news`` target and then edit as you see
   fit)
#. Run ``make distcheck``, fix any issues and commit.
#. Commit NEWS as ``"release 3.X.Y"`` and push
#. Tag with: ``git tag -s 3.X.Y -m "release 3.X.Y"``
#. Push tag with: ``git push origin 3.X.Y``
#. In case of a stable release ``python3 setup.py distcheck`` and
   ``python3 setup.py sdist register upload``
#. Commit post-release version bump to configure.ac
#. Upload tarball: ``scp pygobject-3.X.Y.tar.gz user@master.gnome.org:``
#. Install tarball:
   ``ssh user@master.gnome.org 'ftpadmin install pygobject-3.X.Y.tar.gz'``
#. In case the release happens on a stable branch copy the NEWS changes to
   the master branch

Based on https://wiki.gnome.org/MaintainersCorner/Releasing


Branching
---------

Each cycle after the feature freeze, we create a stable branch so development
can continue in the master branch unaffected by the freezes.

#. Create the branch locally with: ``git checkout -b pygobject-3-2``
#. Push new branch: ``git push origin pygobject-3-2``
#. In master, update configure.ac to what will be the next version number
   (3.3.0)
