# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

from __future__ import absolute_import

# support overrides in different directories than our gi module
from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

import sys
import os

# we can't have pygobject 2 loaded at the same time we load the internal _gobject
if 'gobject' in sys.modules:
    raise ImportError('When using gi.repository you must not import static '
                      'modules like "gobject". Please change all occurrences '
                      'of "import gobject" to "from gi.repository import GObject".')

from ._gi import _gobject
from ._gi import _API
from ._gi import Repository
from ._gi import PyGIDeprecationWarning

_API = _API  # pyflakes
PyGIDeprecationWarning = PyGIDeprecationWarning

_versions = {}
_overridesdir = os.path.join(os.path.dirname(__file__), 'overrides')

version_info = _gobject.pygobject_version[:]
__version__ = "{0}.{1}.{2}".format(*version_info)


def check_version(version):
    if isinstance(version, str):
        version_list = tuple(map(int, version.split(".")))
    else:
        version_list = version

    if version_list > version_info:
        raise ValueError((
            "pygobject's version %s required, and available version "
            "%s is not recent enough") % (version, __version__)
        )


def require_version(namespace, version):
    repository = Repository.get_default()

    if namespace in repository.get_loaded_namespaces():
        loaded_version = repository.get_version(namespace)
        if loaded_version != version:
            raise ValueError('Namespace %s is already loaded with version %s' %
                             (namespace, loaded_version))

    if namespace in _versions and _versions[namespace] != version:
        raise ValueError('Namespace %s already requires version %s' %
                         (namespace, _versions[namespace]))

    available_versions = repository.enumerate_versions(namespace)
    if not available_versions:
        raise ValueError('Namespace %s not available' % namespace)

    if version not in available_versions:
        raise ValueError('Namespace %s not available for version %s' %
                         (namespace, version))

    _versions[namespace] = version


def get_required_version(namespace):
    return _versions.get(namespace, None)
