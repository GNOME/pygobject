.. include:: ../icons.rst

==================================
Testing and Continuous Integration
==================================

To get automated tests of GTK+ code running on a headless server use Xvfb
(virtual framebuffer X server). It provides the ``xvfb-run -a`` command which
creates a temporary X server without the need for any real display hardware.

::

    xvfb-run -a python my_script.py


Continuous Integration using Travis CI / CircleCI
-------------------------------------------------

Travis CI uses a rather old Ubuntu and thus the supported GTK+ is at 3.10 and
PyGObject is at 3.12. If that's enough for you then have a look at our Travis
CI example project:

    |github-logo| https://github.com/pygobject/pygobject-travis-ci-examples

    .. image:: https://travis-ci.org/pygobject/pygobject-travis-ci-examples.svg?branch=master
        :target: https://travis-ci.org/pygobject/pygobject-travis-ci-examples

To get newer PyGObject, GTK+, etc. working on `Travis CI
<https://travis-ci.org>`__ or `CircleCI <https://circleci.com>`__ you can use
Docker with an image of your choosing. Have a look at our Docker example
project which runs tests on various Debian, Ubuntu and Fedora versions:

    |github-logo| https://github.com/pygobject/pygobject-travis-ci-docker-examples

    .. image:: https://travis-ci.org/pygobject/pygobject-travis-ci-docker-examples.svg?branch=master
        :target: https://travis-ci.org/pygobject/pygobject-travis-ci-docker-examples

    .. image:: https://circleci.com/gh/pygobject/pygobject-travis-ci-docker-examples.svg?style=shield
        :target: https://circleci.com/gh/pygobject/pygobject-travis-ci-docker-examples
