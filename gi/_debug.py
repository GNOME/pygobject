# Copyright (C) 2026 Arjan Molenaar <amolenaar@gnopme.org>
#
#   _debug.py: expose debug information.
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

from enum import IntFlag
from ._gi import set_debug_flags


# Keep in sync with PyGIDebugFlags in pygi-debug.h
class DebugFlags(IntFlag):
    MARSHALLER = 1
    LIFECYCLE = 2


debug_options = {
    "marshaller": (
        DebugFlags.MARSHALLER,
        "Information about how data is marshalled to and from Python",
    ),
    "lifecycle": (
        DebugFlags.LIFECYCLE,
        "Information about Python object creation and destruction",
    ),
    "help": (0, "Help"),
}


def set_debug_options(options: str) -> None:
    flags = 0

    for option in options.split(","):
        dbg_opt = debug_options.get(option)
        if dbg_opt:
            flags |= dbg_opt[0]

        if option == "help":
            print("PYGI_DEBUG options, separated by a comma (,):")  # noqa: T201
            for name, dbg_opt in debug_options.items():
                print(f"  {name: <10}: {dbg_opt[1]}")  # noqa: T201

    set_debug_flags(flags)
