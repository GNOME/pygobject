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
from .module import get_introspection_module
from .overrides import load_overrides


repository = Repository.get_default()

# only for backwards compatibility
modules = {}


def _get_all_dependencies(namespace):
    """Like get_dependencies() but will recurse and get all dependencies.
    The namespace has to be loaded before this can be called.

    ::

        _get_all_dependencies('Gtk') -> ['Atk-1.0', 'GObject-2.0', ...]
    """

    todo = repository.get_dependencies(namespace)
    dependencies = []

    while todo:
        current = todo.pop()
        if current in dependencies:
            continue
        ns, version = current.split("-", 1)
        todo.extend(repository.get_dependencies(ns))
        dependencies.append(current)

    return dependencies


# See _check_require_version()
_active_imports = []
_implicit_required = {}


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

    global _active_imports, _implicit_required

    # This keeps track of the recursion level so we only check for
    # explicitly imported namespaces and not the ones imported in overrides
    _active_imports.append(namespace)

    try:
        yield
    except:
        raise
    else:
        # Keep track of all dependency versions forced due to this import, so
        # we don't warn for them in the future. This mirrors the import
        # behavior where importing will get an older version if a previous
        # import depended on it.
        for dependency in _get_all_dependencies(namespace):
            ns, version = dependency.split("-", 1)
            _implicit_required[ns] = version
    finally:
        _active_imports.remove(namespace)

    # Warn in case:
    #  * this namespace was explicitly imported
    #  * the version wasn't forced using require_version()
    #  * the version wasn't forced implicitly by a previous import
    #  * this namespace isn't part of glib (we have bigger problems if
    #    versions change there)
    is_explicit_import = not _active_imports
    version_required = gi.get_required_version(namespace) is not None
    version_implicit = namespace in _implicit_required
    is_in_glib = namespace in ("GLib", "GObject", "Gio")

    if is_explicit_import and not version_required and \
            not version_implicit and not is_in_glib:
        version = repository.get_version(namespace)
        warnings.warn(
            "%(namespace)s was imported without specifying a version first. "
            "Use gi.require_version('%(namespace)s', '%(version)s') before "
            "import to ensure that the right version gets loaded."
            % {"namespace": namespace, "version": version},
            ImportWarning, stacklevel=stacklevel)


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
