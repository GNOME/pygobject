================
Maintainer Guide
================

Making a Release
----------------

#. Make sure the `meson.build` file contains the right version number
#. Update the NEWS file
#. Build new version using ``python3 -m build --sdist``
#. Commit NEWS as ``"release 3.X.Y"`` and push
#. Tag with: ``git tag -s 3.X.Y -m "release 3.X.Y"``
#. Push tag with: ``git push origin 3.X.Y``
#. In case of a stable release, upload to PyPI:
   ``twine upload dist/pygobject-3.X.Y.tar.gz``
#. Commit post-release version bump to pyproject.toml
#. In case the release happens on a stable branch copy the NEWS changes to
   the main branch


Branching
---------

Each cycle after the feature freeze, we create a stable branch so development
can continue in the main branch unaffected by the freezes.

#. Create the branch locally with: ``git checkout -b pygobject-3-2``
#. Push new branch: ``git push origin pygobject-3-2``
#. In main, update pyproject.toml to what will be the next version number
   (3.3.0)
