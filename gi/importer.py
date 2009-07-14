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

import sys
import new
import gobject

from ._gi import Repository
from .module import DynamicModule


repository = Repository.get_default()
sys.modules['GObject'] = gobject


class DynamicImporter(object):

    # Note: see PEP302 for the Importer Protocol implemented below.

    def find_module(self, fullname, path=None):
        # Avoid name clashes, and favour static bindings.
        if fullname == 'cairo':
            return None

        if repository.require(fullname):
            return self

    def load_module(self, fullname):
        if fullname in sys.modules:
            return sys.modules[name]

        # Look for an overrides module
        overrides_name = 'gi.overrides.%s' % fullname
        try:
            overrides_type_name = '%sModule' % fullname
            overrides_module = __import__(overrides_name, {}, {}, [overrides_type_name])
            module_type = getattr(overrides_module, overrides_type_name)
        except ImportError, e:
            module_type = DynamicModule

        module = module_type.__new__(module_type)
        module.__dict__ = {
            '__file__': '<%s>' % fullname,
            '__name__': fullname,
            '__loader__': self
        }

        sys.modules[fullname] = module

        module.__init__()

        return module


def install_importhook():
    sys.meta_path.append(DynamicImporter())

