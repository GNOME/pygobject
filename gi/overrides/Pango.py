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

from __future__ import annotations

import typing

from ..overrides import override
from ..module import get_introspection_module

# Typing relies on https://github.com/pygobject/pygobject-stubs.
if typing.TYPE_CHECKING:
    # Import stubs for type checking this file.
    # FIXME: For IDE typing support, you need to copy
    # pygobject-stubs/src/gi-stubs/repository/*.pyi to gi/repository/
    from gi.repository import Pango
    from typing_extensions import Self

    # Type annotations cannot have `Pango.` prefix because they are copied into
    # Pango stubs module which cannot refer to itself. Use type aliases.
    Context = Pango.Context
else:
    from gi.module import get_introspection_module

    Pango = get_introspection_module("Pango")

__all__ = []


class FontDescription(Pango.FontDescription):
    @staticmethod
    def __new__(cls: type[Self], string: str | None = None) -> Self:
        if string is not None:
            return Pango.font_description_from_string(string)  # type: ignore[return-value]
        return Pango.FontDescription.new()  # type: ignore[return-value]


FontDescription = override(FontDescription)
__all__.append("FontDescription")


class Layout(Pango.Layout):
    def __new__(cls: type[Self], context: Context) -> Self:
        return Pango.Layout.new(context)  # type: ignore[return-value]

    def set_markup(self, text: str, length: int = -1) -> None:
        super().set_markup(text, length)

    def set_text(self, text: str, length: int = -1) -> None:
        super().set_text(text, length)


Layout = override(Layout)
__all__.append("Layout")
