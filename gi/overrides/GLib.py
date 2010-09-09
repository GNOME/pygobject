# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2009 Johan Dahlin <johan@gnome.org>
#               2010 Simon van der Linden <svdlinden@src.gnome.org>
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

import ctypes

from ..types import override
from ..importer import modules
from .._gi import variant_new_tuple

GLib = modules['GLib']

__all__ = []

class Variant(GLib.Variant):
    def __repr__(self):
        return '<GLib.Variant(%s)>' % getattr(self, 'print')(True)

@classmethod
def new_tuple(cls, *elements):
    return variant_new_tuple(elements)

def get_string(self):
    value, length = GLib.Variant.get_string(self)
    return value

setattr(Variant, 'new_tuple', new_tuple)
setattr(Variant, 'get_string', get_string)

__all__.append('Variant')

