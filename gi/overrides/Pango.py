# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Paolo Borelli <pborelli@gnome.org>
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

from ..overrides import override
from ..importer import modules

Pango = modules['Pango']._introspection_module

__all__ = []

class FontDescription(Pango.FontDescription):

    def __new__(cls, string=None):
        if string is not None:
            return Pango.font_description_from_string (string)
        else:
            return Pango.FontDescription.__new__(cls)

FontDescription = override(FontDescription)
__all__.append('FontDescription')

class Layout(Pango.Layout):

    def __new__(cls, context):
        return Pango.Layout.new(context)

    def __init__(self, context, **kwds):
        # simply discard 'context', since it was set by
        # __new__ and it is not a PangoLayout property
        super(Layout, self).__init__(**kwds)

Layout = override(Layout)
__all__.append('Layout')

