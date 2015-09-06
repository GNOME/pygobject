# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
#               2015 Christoph Reiter
#
#   importer.py: dynamic importer for introspected libraries.
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
import sys
import warnings
from contextlib import contextmanager

import gi
from ._gi import Repository
from ._gi import PyGIWarning
from .module import get_introspection_module
from .overrides import load_overrides


repository = Repository.get_default()

# only for backwards compatibility
modules = {}


@contextmanager
def _check_require_version(namespace, stacklevel):
    """A context manager which tries to give helpful warnings
    about missing gi.require_version() which could potentially
    break code if only an older version than expected is installed
    or a new version gets introduced.

    ::

        with _check_require_version("Gtk", stacklevel):
            load_namespace_and_overrides()
    """

    was_loaded = repository.is_registered(namespace)

    yield

    if was_loaded:
        # it was loaded before by another import which depended on this
        # namespace or by C code like libpeas
        return

    if namespace in ("GLib", "GObject", "Gio"):
        # part of glib (we have bigger problems if versions change there)
        return

    if gi.get_required_version(namespace) is not None:
        # the version was forced using require_version()
        return

    version = repository.get_version(namespace)
    warnings.warn(
        "%(namespace)s was imported without specifying a version first. "
        "Use gi.require_version('%(namespace)s', '%(version)s') before "
        "import to ensure that the right version gets loaded."
        % {"namespace": namespace, "version": version},
        PyGIWarning, stacklevel=stacklevel)


class DynamicImporter(object):

    # Note: see PEP302 for the Importer Protocol implemented below.

    def __init__(self, path):
        self.path = path

    def find_module(self, fullname, path=None):
        if not fullname.startswith(self.path):
            return

        path, namespace = fullname.rsplit('.', 1)
        if path != self.path:
            return

        if repository.enumerate_versions(namespace):
            return self
        else:
            raise ImportError('cannot import name %s, '
                              'introspection typelib not found' % namespace)

    def load_module(self, fullname):
        if fullname in sys.modules:
            return sys.modules[fullname]

        path, namespace = fullname.rsplit('.', 1)

        # we want the warning to point to the line doing the import
        if sys.version_info >= (3, 0):
            stacklevel = 10
        else:
            stacklevel = 4
        with _check_require_version(namespace, stacklevel=stacklevel):
            introspection_module = get_introspection_module(namespace)
            dynamic_module = load_overrides(introspection_module)

        dynamic_module.__file__ = '<%s>' % fullname
        dynamic_module.__loader__ = self
        sys.modules[fullname] = dynamic_module

        return dynamic_module
