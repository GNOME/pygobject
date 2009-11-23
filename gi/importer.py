# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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
import gobject

from ._gi import Repository, RepositoryError
from .module import DynamicModule


repository = Repository.get_default()


class DynamicImporter(object):

    # Note: see PEP302 for the Importer Protocol implemented below.

    def __init__(self, path):
        self.path = path

    def _create_module(self, module_type, name, namespace):
        module = module_type.__new__(module_type)
        module.__dict__ = {
            '__file__': '<%s>' % name,
            '__name__': name,
            '__namespace__': namespace,
            '__loader__': self
        }
        module.__init__()
        return module

    def find_module(self, fullname, path=None):
        if not fullname.startswith(self.path):
            return

        path, namespace = fullname.rsplit('.', 1)
        if path != self.path:
            return
        try:
            repository.require(namespace)
        except RepositoryError:
            pass
        else:
            return self

    def load_module(self, fullname):
        if fullname in sys.modules:
            return sys.modules[name]

        path, namespace = fullname.rsplit('.', 1)

        # Workaround for GObject
        if namespace == 'GObject':
            sys.modules[fullname] = gobject
            return gobject

        module_type = DynamicModule
        module = self._create_module(module_type, fullname, namespace)
        sys.modules[fullname] = module

        # Look for an overrides module
        overrides_name = 'gi.overrides.%s' % namespace
        overrides_type_name = '%sModule' % namespace
        try:

            overrides_module = __import__(overrides_name, fromlist=[overrides_type_name])
            module_type = getattr(overrides_module, overrides_type_name)
        except ImportError, e:
            pass

        if module_type is not DynamicModule:
            module = self._create_module(module_type, fullname, namespace)
            sys.modules[fullname] = module

        return module

