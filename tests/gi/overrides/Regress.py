# Copyright (C) 2012 Martin Pitt <martinpitt@gnome.org>
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

from ..importer import get_introspection_module
from ..overrides import override

Regress = get_introspection_module("Regress")

REGRESS_OVERRIDE = 42


class Bitmask(Regress.Bitmask):
    """Replicate override of Bitmask (uint64), similar to GStreamer."""

    def __init__(self, v):
        if not isinstance(v, int):
            raise TypeError(f"{type(v)} is not an int.")

        self.v = int(v)

    def __str__(self):
        return hex(self.v)

    def __eq__(self, other):
        return self.v == other


Bitmask = override(Bitmask)

__all__ = ["REGRESS_OVERRIDE", "Bitmask"]
