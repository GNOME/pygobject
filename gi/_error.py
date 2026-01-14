# Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
#
#   _error.py: GError Python implementation
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


# NOTE: This file should not have any dependencies on introspection libs
# like gi.repository.GLib because it would cause a circular dependency.
# Developers wanting to use the GError class in their applications should
# use gi.repository.GLib.GError

from __future__ import annotations

import typing

if typing.TYPE_CHECKING:
    from typing_extensions import Self


class GError(RuntimeError):
    message: str
    domain: str
    code: int

    def __init__(
        self, message: str = "unknown error", domain: str = "pygi-error", code: int = 0
    ) -> None:
        super().__init__(message)
        self.message = message
        self.domain = domain
        self.code = code

    def __str__(self) -> str:
        return f"{self.domain:s}: {self.message:s} ({self.code:d})"

    def __repr__(self) -> str:
        return f"{GError.__module__.rsplit('.', 1)[-1]:s}.{GError.__name__:s}('{self.message:s}', '{self.domain:s}', {self.code:d})"

    def copy(self) -> Self:
        return GError(self.message, self.domain, self.code)

    def matches(self, domain: str | int, code: int) -> bool:
        """Placeholder that will be monkey patched in GLib overrides."""
        raise NotImplementedError

    @staticmethod
    def new_literal(domain: int, message: str, code: int) -> Self:
        """Placeholder that will be monkey patched in GLib overrides."""
        raise NotImplementedError
