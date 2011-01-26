# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Ignacio Casal Quinteiro <icq@gnome.org>
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

Gio = modules['Gio']._introspection_module

__all__ = []

class FileEnumerator(Gio.FileEnumerator):
    def __iter__(self):
        return self

    def __next__(self):
        file_info = self.next_file(None)

        if file_info is not None:
            return file_info
        else:
            raise StopIteration

    # python 2 compat for the iter protocol
    next = __next__


FileEnumerator = override(FileEnumerator)
__all__.append('FileEnumerator')
