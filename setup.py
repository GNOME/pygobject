#!/usr/bin/env python


import os
import re
import subprocess
import sys

from distutils.command.build import build as orig_build
from setuptools.command.build_ext import build_ext as orig_build_ext
from setuptools.command.build_py import build_py as orig_build_py
from setuptools import setup, Extension


with open("configure.ac", "r") as h:
    version = ".".join(re.findall("pygobject_[^\s]+_version,\s*(\d+)\)", h.read()))


def makedirs(dirpath):
    """Safely make directories

    By default, os.makedirs fails if the directory already exists.

    Python 3.2 introduced the `exist_ok` argument, but we can't use it because
    we want to keep supporting Python 2 for some time.
    """
    import errno

    try:
        os.makedirs(dirpath)

    except OSError as e:
        if e.errno == errno.EEXIST:
            return

        raise


class Build(orig_build):
    """Dummy version of distutils build which runs an Autotools build system
    instead.
    """
    def run(self):
        srcdir = os.getcwd()
        builddir = os.path.join(srcdir, self.build_temp)
        makedirs(builddir)
        configure = os.path.join(srcdir, 'configure')

        if not os.path.exists(configure):
            configure = os.path.join(srcdir, 'autogen.sh')

        subprocess.check_call([
                configure,
                'PYTHON=%s' % sys.executable,
                # Put the documentation, etc. out of the way: we only want
                # the Python code and extensions
                '--prefix=' + os.path.join(builddir, 'prefix'),
            ],
            cwd=builddir)
        make_args = [
            'pythondir=%s' % os.path.join(srcdir, self.build_lib),
            'pyexecdir=%s' % os.path.join(srcdir, self.build_lib),
        ]
        subprocess.check_call(['make', '-C', builddir] + make_args)
        subprocess.check_call(['make', '-C', builddir, 'install'] + make_args)


class BuildExt(orig_build_ext):
    def run(self):
        pass


class BuildPy(orig_build_py):
    def run(self):
        pass


setup(
    name='pygobject',
    version=version,
    description='Python bindings for GObject Introspection',
    maintainer='The pygobject maintainers',
    maintainer_email='http://mail.gnome.org/mailman/listinfo/python-hackers-list',
    download_url='http://download.gnome.org/sources/pygobject/',
    url='https://wiki.gnome.org/Projects/PyGObject',
    packages=['gi', 'pygtkcompat'],
    ext_modules=[
        Extension(
            '_gi', sources=['gi/gimodule.c'])
        ],
    license='LGPL',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Programming Language :: C',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: Implementation :: CPython',
    ],
    cmdclass={
        'build': Build,
        'build_py': BuildPy,
        'build_ext': BuildExt,
    },
)
