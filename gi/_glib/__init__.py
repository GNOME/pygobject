# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2006-2012 Johan Dahlin
#
#   glib/__init__.py: initialisation file for glib module
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
# USA

from . import _glib

# Internal API
_PyGLib_API = _glib._PyGLib_API

# Types
GError = _glib.GError
OptionContext = _glib.OptionContext
OptionGroup = _glib.OptionGroup
Pid = _glib.Pid
PollFD = _glib.PollFD

OPTION_ERROR = _glib.OPTION_ERROR
OPTION_ERROR_BAD_VALUE = _glib.OPTION_ERROR_BAD_VALUE
OPTION_ERROR_FAILED = _glib.OPTION_ERROR_FAILED
OPTION_ERROR_UNKNOWN_OPTION = _glib.OPTION_ERROR_UNKNOWN_OPTION
OPTION_FLAG_FILENAME = _glib.OPTION_FLAG_FILENAME
OPTION_FLAG_HIDDEN = _glib.OPTION_FLAG_HIDDEN
OPTION_FLAG_IN_MAIN = _glib.OPTION_FLAG_IN_MAIN
OPTION_FLAG_NOALIAS = _glib.OPTION_FLAG_NOALIAS
OPTION_FLAG_NO_ARG = _glib.OPTION_FLAG_NO_ARG
OPTION_FLAG_OPTIONAL_ARG = _glib.OPTION_FLAG_OPTIONAL_ARG
OPTION_FLAG_REVERSE = _glib.OPTION_FLAG_REVERSE
OPTION_REMAINING = _glib.OPTION_REMAINING

# Functions
filename_from_utf8 = _glib.filename_from_utf8
get_current_time = _glib.get_current_time
glib_version = _glib.glib_version
pyglib_version = _glib.pyglib_version
spawn_async = _glib.spawn_async
threads_init = _glib.threads_init
