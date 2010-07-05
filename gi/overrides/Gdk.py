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

from ..types import override
from ..importer import modules

Gdk = modules['Gdk']

__all__ = []

class Color(Gdk.Color):

    def __init__(self, red, green, blue):
        Gdk.Color.__init__(self)
        self.red = red
        self.green = green
        self.blue = blue

    def __new__(cls, *args, **kwargs):
        return Gdk.Color.__new__(cls)

    def __repr__(self):
        return '<Gdk.Color(red=%d, green=%d, blue=%d)>' % (self.red, self.green, self.blue)

Color = override(Color)
__all__.append('Color')

class Drawable(Gdk.Drawable):
    def cairo_create(self):
        return Gdk.cairo_create(self)

Drawable = override(Drawable)
__all__.append('Drawable')


import sys

initialized, argv = Gdk.init_check(sys.argv)
if not initialized:
    raise RuntimeError("Gdk couldn't be initialized")
