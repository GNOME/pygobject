# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2012 Canonical Ltd.
# Author: Martin Pitt <martin.pitt@ubuntu.com>
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

import gi.overrides
from gi.repository import GLib

__all__ = []

# API aliases for backwards compatibility
for name in ['markup_escape_text', 'get_application_name',
             'set_application_name', 'get_prgname', 'set_prgname',
             'main_depth', 'filename_display_basename',
             'filename_display_name', 'uri_list_extract_uris',
             'MainLoop', 'MainContext', 'main_context_default',
             'source_remove', 'Source', 'Idle', 'Timeout', 'PollFD',
             'idle_add', 'timeout_add', 'timeout_add_seconds']:
    globals()[name] = gi.overrides.deprecated(getattr(GLib, name), 'GLib.' + name)
    __all__.append(name)

# constants are also deprecated, but cannot mark them as such
for name in ['PRIORITY_DEFAULT', 'PRIORITY_DEFAULT_IDLE', 'PRIORITY_HIGH',
             'PRIORITY_HIGH_IDLE', 'PRIORITY_LOW',
             'IO_IN', 'IO_OUT', 'IO_PRI', 'IO_ERR', 'IO_HUP', 'IO_NVAL',
             'IO_STATUS_ERROR', 'IO_STATUS_NORMAL', 'IO_STATUS_EOF',
             'IO_STATUS_AGAIN', 'IO_FLAG_APPEND', 'IO_FLAG_NONBLOCK',
             'IO_FLAG_IS_READABLE', 'IO_FLAG_IS_WRITEABLE',
             'IO_FLAG_IS_SEEKABLE', 'IO_FLAG_MASK', 'IO_FLAG_GET_MASK',
             'IO_FLAG_SET_MASK']:
    globals()[name] = getattr(GLib, name)
    __all__.append(name)
