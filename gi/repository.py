# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2007-2009 Johan Dahlin <johan@gnome.org>
#
#   repository.py: Repository wrapper.
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

import gobject

from ._gi import Repository

class _Repository(object):
    def __init__(self):
        self._repo = Repository.getDefault()
        self._modules = {}

    def register(self, module, namespace, filename):
        self._modules[namespace] = module

    def require(self, namespace):
        return self._repo.require(namespace)

    def get_module(self, namespace):
        return self._modules.get(namespace)

    def get_by_name(self, namespace, name):
        return self._repo.findByName(namespace, name)

    def get_by_typename(self, typename):
        raise NotImplemented

    def get_infos(self, namespace):
        return self._repo.getInfos(namespace)

    def get_c_prefix(self, namespace):
        return self._repo.getCPrefix(namespace)

repository = _Repository()
repository.register(gobject, 'GObject', None)
