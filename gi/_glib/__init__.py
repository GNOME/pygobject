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
IOChannel = _glib.IOChannel
Idle = _glib.Idle
MainContext = _glib.MainContext
MainLoop = _glib.MainLoop
OptionContext = _glib.OptionContext
OptionGroup = _glib.OptionGroup
Pid = _glib.Pid
PollFD = _glib.PollFD
Source = _glib.Source
Timeout = _glib.Timeout

# Constants
IO_ERR = _glib.IO_ERR
IO_FLAG_APPEND = _glib.IO_FLAG_APPEND
IO_FLAG_GET_MASK = _glib.IO_FLAG_GET_MASK
IO_FLAG_IS_READABLE = _glib.IO_FLAG_IS_READABLE
IO_FLAG_IS_SEEKABLE = _glib.IO_FLAG_IS_SEEKABLE
IO_FLAG_IS_WRITEABLE = _glib.IO_FLAG_IS_WRITEABLE
IO_FLAG_MASK = _glib.IO_FLAG_MASK
IO_FLAG_NONBLOCK = _glib.IO_FLAG_NONBLOCK
IO_FLAG_SET_MASK = _glib.IO_FLAG_SET_MASK
IO_HUP = _glib.IO_HUP
IO_IN = _glib.IO_IN
IO_NVAL = _glib.IO_NVAL
IO_OUT = _glib.IO_OUT
IO_PRI = _glib.IO_PRI
IO_STATUS_AGAIN = _glib.IO_STATUS_AGAIN
IO_STATUS_EOF = _glib.IO_STATUS_EOF
IO_STATUS_ERROR = _glib.IO_STATUS_ERROR
IO_STATUS_NORMAL = _glib.IO_STATUS_NORMAL
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
PRIORITY_DEFAULT = _glib.PRIORITY_DEFAULT
PRIORITY_DEFAULT_IDLE = _glib.PRIORITY_DEFAULT_IDLE
PRIORITY_HIGH = _glib.PRIORITY_HIGH
PRIORITY_HIGH_IDLE = _glib.PRIORITY_HIGH_IDLE
PRIORITY_LOW = _glib.PRIORITY_LOW
SPAWN_CHILD_INHERITS_STDIN = _glib.SPAWN_CHILD_INHERITS_STDIN
SPAWN_DO_NOT_REAP_CHILD = _glib.SPAWN_DO_NOT_REAP_CHILD
SPAWN_FILE_AND_ARGV_ZERO = _glib.SPAWN_FILE_AND_ARGV_ZERO
SPAWN_LEAVE_DESCRIPTORS_OPEN = _glib.SPAWN_LEAVE_DESCRIPTORS_OPEN
SPAWN_SEARCH_PATH = _glib.SPAWN_SEARCH_PATH
SPAWN_STDERR_TO_DEV_NULL = _glib.SPAWN_STDERR_TO_DEV_NULL
SPAWN_STDOUT_TO_DEV_NULL = _glib.SPAWN_STDOUT_TO_DEV_NULL
USER_DIRECTORY_DESKTOP = _glib.USER_DIRECTORY_DESKTOP
USER_DIRECTORY_DOCUMENTS = _glib.USER_DIRECTORY_DOCUMENTS
USER_DIRECTORY_DOWNLOAD = _glib.USER_DIRECTORY_DOWNLOAD
USER_DIRECTORY_MUSIC = _glib.USER_DIRECTORY_MUSIC
USER_DIRECTORY_PICTURES = _glib.USER_DIRECTORY_PICTURES
USER_DIRECTORY_PUBLIC_SHARE = _glib.USER_DIRECTORY_PUBLIC_SHARE
USER_DIRECTORY_TEMPLATES = _glib.USER_DIRECTORY_TEMPLATES
USER_DIRECTORY_VIDEOS = _glib.USER_DIRECTORY_VIDEOS

# Functions
child_watch_add = _glib.child_watch_add
filename_display_basename = _glib.filename_display_basename
filename_display_name = _glib.filename_display_name
filename_from_utf8 = _glib.filename_from_utf8
find_program_in_path = _glib.find_program_in_path
get_application_name = _glib.get_application_name
get_current_time = _glib.get_current_time
get_prgname = _glib.get_prgname
get_system_config_dirs = _glib.get_system_config_dirs
get_system_data_dirs = _glib.get_system_data_dirs
get_user_cache_dir = _glib.get_user_cache_dir
get_user_config_dir = _glib.get_user_config_dir
get_user_data_dir = _glib.get_user_data_dir
get_user_special_dir = _glib.get_user_special_dir
glib_version = _glib.glib_version
idle_add = _glib.idle_add
io_add_watch = _glib.io_add_watch
main_context_default = _glib.main_context_default
main_depth = _glib.main_depth
markup_escape_text = _glib.markup_escape_text
pyglib_version = _glib.pyglib_version
set_application_name = _glib.set_application_name
set_prgname = _glib.set_prgname
source_remove = _glib.source_remove
spawn_async = _glib.spawn_async
threads_init = _glib.threads_init
timeout_add = _glib.timeout_add
timeout_add_seconds = _glib.timeout_add_seconds
uri_list_extract_uris = _glib.uri_list_extract_uris
