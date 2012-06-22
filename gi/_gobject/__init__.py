# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2006-2012  Johan Dahlin
#
#   gobject/__init__.py: initialisation file for gobject module
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

# this can go when things are a little further along

import sys

# we can't have pygobject 2 loaded at the same time we load the internal _gobject
if 'gobject' in sys.modules:
    raise ImportError('When using gi.repository you must not import static modules like "gobject". Please change all occurrences of "import gobject" to "from gi.repository import GObject".')

from .. import _glib
from . import _gobject
from . import constants
from .propertyhelper import Property

GBoxed = _gobject.GBoxed
GEnum = _gobject.GEnum
GFlags = _gobject.GFlags
GInterface = _gobject.GInterface
GObject = _gobject.GObject
GObjectWeakRef = _gobject.GObjectWeakRef
GParamSpec = _gobject.GParamSpec
GPointer = _gobject.GPointer
GType = _gobject.GType
PARAM_CONSTRUCT = _gobject.PARAM_CONSTRUCT
PARAM_CONSTRUCT_ONLY = _gobject.PARAM_CONSTRUCT_ONLY
PARAM_LAX_VALIDATION = _gobject.PARAM_LAX_VALIDATION
PARAM_READABLE = _gobject.PARAM_READABLE
PARAM_READWRITE = _gobject.PARAM_READWRITE
PARAM_WRITABLE = _gobject.PARAM_WRITABLE
SIGNAL_ACTION = _gobject.SIGNAL_ACTION
SIGNAL_DETAILED = _gobject.SIGNAL_DETAILED
SIGNAL_NO_HOOKS = _gobject.SIGNAL_NO_HOOKS
SIGNAL_NO_RECURSE = _gobject.SIGNAL_NO_RECURSE
SIGNAL_RUN_CLEANUP = _gobject.SIGNAL_RUN_CLEANUP
SIGNAL_RUN_FIRST = _gobject.SIGNAL_RUN_FIRST
SIGNAL_RUN_LAST = _gobject.SIGNAL_RUN_LAST
TYPE_GSTRING = _gobject.TYPE_GSTRING
TYPE_INVALID = _gobject.TYPE_INVALID
Warning = _gobject.Warning
_PyGObject_API = _gobject._PyGObject_API
add_emission_hook = _gobject.add_emission_hook
features = _gobject.features
list_properties = _gobject.list_properties
new = _gobject.new
pygobject_version = _gobject.pygobject_version
remove_emission_hook = _gobject.remove_emission_hook
signal_accumulator_true_handled = _gobject.signal_accumulator_true_handled
signal_list_ids = _gobject.signal_list_ids
signal_list_names = _gobject.signal_list_names
signal_lookup = _gobject.signal_lookup
signal_name = _gobject.signal_name
signal_new = _gobject.signal_new
signal_query = _gobject.signal_query
threads_init = _gobject.threads_init
type_children = _gobject.type_children
type_from_name = _gobject.type_from_name
type_interfaces = _gobject.type_interfaces
type_is_a = _gobject.type_is_a
type_name = _gobject.type_name
type_parent = _gobject.type_parent
type_register = _gobject.type_register

spawn_async = _glib.spawn_async
idle_add = _glib.idle_add
timeout_add = _glib.timeout_add
timeout_add_seconds = _glib.timeout_add_seconds
io_add_watch = _glib.io_add_watch
source_remove = _glib.source_remove
child_watch_add = _glib.child_watch_add
markup_escape_text = _glib.markup_escape_text
get_current_time = _glib.get_current_time
filename_display_name = _glib.filename_display_name
filename_display_basename = _glib.filename_display_basename
filename_from_utf8 = _glib.filename_from_utf8
get_application_name = _glib.get_application_name
set_application_name = _glib.set_application_name
get_prgname = _glib.get_prgname
set_prgname = _glib.set_prgname
main_depth = _glib.main_depth
Pid = _glib.Pid
GError = _glib.GError
glib_version = _glib.glib_version
MainLoop = _glib.MainLoop
MainContext = _glib.MainContext
main_context_default = _glib.main_context_default
IOChannel = _glib.IOChannel
Source = _glib.Source
Idle = _glib.Idle
Timeout = _glib.Timeout
PollFD = _glib.PollFD
OptionGroup = _glib.OptionGroup
OptionContext = _glib.OptionContext
uri_list_extract_uris = _glib.uri_list_extract_uris

SPAWN_LEAVE_DESCRIPTORS_OPEN = _glib.SPAWN_LEAVE_DESCRIPTORS_OPEN
SPAWN_DO_NOT_REAP_CHILD = _glib.SPAWN_DO_NOT_REAP_CHILD
SPAWN_SEARCH_PATH = _glib.SPAWN_SEARCH_PATH
SPAWN_STDOUT_TO_DEV_NULL = _glib.SPAWN_STDOUT_TO_DEV_NULL
SPAWN_STDERR_TO_DEV_NULL = _glib.SPAWN_STDERR_TO_DEV_NULL
SPAWN_CHILD_INHERITS_STDIN = _glib.SPAWN_CHILD_INHERITS_STDIN
SPAWN_FILE_AND_ARGV_ZERO = _glib.SPAWN_FILE_AND_ARGV_ZERO
PRIORITY_HIGH = _glib.PRIORITY_HIGH
PRIORITY_DEFAULT = _glib.PRIORITY_DEFAULT
PRIORITY_HIGH_IDLE = _glib.PRIORITY_HIGH_IDLE
PRIORITY_DEFAULT_IDLE = _glib.PRIORITY_DEFAULT_IDLE
PRIORITY_LOW = _glib.PRIORITY_LOW
IO_IN = _glib.IO_IN
IO_OUT = _glib.IO_OUT
IO_PRI = _glib.IO_PRI
IO_ERR = _glib.IO_ERR
IO_HUP = _glib.IO_HUP
IO_NVAL = _glib.IO_NVAL
IO_STATUS_ERROR = _glib.IO_STATUS_ERROR
IO_STATUS_NORMAL = _glib.IO_STATUS_NORMAL
IO_STATUS_EOF = _glib.IO_STATUS_EOF
IO_STATUS_AGAIN = _glib.IO_STATUS_AGAIN
IO_FLAG_APPEND = _glib.IO_FLAG_APPEND
IO_FLAG_NONBLOCK = _glib.IO_FLAG_NONBLOCK
IO_FLAG_IS_READABLE = _glib.IO_FLAG_IS_READABLE
IO_FLAG_IS_WRITEABLE = _glib.IO_FLAG_IS_WRITEABLE
IO_FLAG_IS_SEEKABLE = _glib.IO_FLAG_IS_SEEKABLE
IO_FLAG_MASK = _glib.IO_FLAG_MASK
IO_FLAG_GET_MASK = _glib.IO_FLAG_GET_MASK
IO_FLAG_SET_MASK = _glib.IO_FLAG_SET_MASK
OPTION_FLAG_HIDDEN = _glib.OPTION_FLAG_HIDDEN
OPTION_FLAG_IN_MAIN = _glib.OPTION_FLAG_IN_MAIN
OPTION_FLAG_REVERSE = _glib.OPTION_FLAG_REVERSE
OPTION_FLAG_NO_ARG = _glib.OPTION_FLAG_NO_ARG
OPTION_FLAG_FILENAME = _glib.OPTION_FLAG_FILENAME
OPTION_FLAG_OPTIONAL_ARG = _glib.OPTION_FLAG_OPTIONAL_ARG
OPTION_FLAG_NOALIAS = _glib.OPTION_FLAG_NOALIAS
OPTION_ERROR_UNKNOWN_OPTION = _glib.OPTION_ERROR_UNKNOWN_OPTION
OPTION_ERROR_BAD_VALUE = _glib.OPTION_ERROR_BAD_VALUE
OPTION_ERROR_FAILED = _glib.OPTION_ERROR_FAILED
OPTION_REMAINING = _glib.OPTION_REMAINING
OPTION_ERROR = _glib.OPTION_ERROR

TYPE_NONE = constants.TYPE_NONE
TYPE_INTERFACE = constants.TYPE_INTERFACE
TYPE_CHAR = constants.TYPE_CHAR
TYPE_UCHAR = constants.TYPE_UCHAR
TYPE_BOOLEAN = constants.TYPE_BOOLEAN
TYPE_INT = constants.TYPE_INT
TYPE_UINT = constants.TYPE_UINT
TYPE_LONG = constants.TYPE_LONG
TYPE_ULONG = constants.TYPE_ULONG
TYPE_INT64 = constants.TYPE_INT64
TYPE_UINT64 = constants.TYPE_UINT64
TYPE_ENUM = constants.TYPE_ENUM
TYPE_FLAGS = constants.TYPE_FLAGS
TYPE_FLOAT = constants.TYPE_FLOAT
TYPE_DOUBLE = constants.TYPE_DOUBLE
TYPE_STRING = constants.TYPE_STRING
TYPE_POINTER = constants.TYPE_POINTER
TYPE_BOXED = constants.TYPE_BOXED
TYPE_PARAM = constants.TYPE_PARAM
TYPE_OBJECT = constants.TYPE_OBJECT
TYPE_PYOBJECT = constants.TYPE_PYOBJECT
TYPE_GTYPE = constants.TYPE_GTYPE
TYPE_UNICHAR = constants.TYPE_UNICHAR
TYPE_STRV = constants.TYPE_STRV
TYPE_VARIANT = constants.TYPE_VARIANT
G_MINFLOAT = constants.G_MINFLOAT
G_MAXFLOAT = constants.G_MAXFLOAT
G_MINDOUBLE = constants.G_MINDOUBLE
G_MAXDOUBLE = constants.G_MAXDOUBLE
G_MINSHORT = constants.G_MINSHORT
G_MAXSHORT = constants.G_MAXSHORT
G_MAXUSHORT = constants.G_MAXUSHORT
G_MININT = constants.G_MININT
G_MAXINT = constants.G_MAXINT
G_MAXUINT = constants.G_MAXUINT
G_MINLONG = constants.G_MINLONG
G_MAXLONG = constants.G_MAXLONG
G_MAXULONG = constants.G_MAXULONG
G_MININT8 = constants.G_MININT8
G_MAXINT8 = constants.G_MAXINT8
G_MAXUINT8 = constants.G_MAXUINT8
G_MININT16 = constants.G_MININT16
G_MAXINT16 = constants.G_MAXINT16
G_MAXUINT16 = constants.G_MAXUINT16
G_MININT32 = constants.G_MININT32
G_MAXINT32 = constants.G_MAXINT32
G_MAXUINT32 = constants.G_MAXUINT32
G_MININT64 = constants.G_MININT64
G_MAXINT64 = constants.G_MAXINT64
G_MAXUINT64 = constants.G_MAXUINT64
G_MAXSIZE = constants.G_MAXSIZE
G_MAXSSIZE = constants.G_MAXSSIZE
G_MINOFFSET = constants.G_MINOFFSET
G_MAXOFFSET = constants.G_MAXOFFSET

from .._glib import option
sys.modules['gi._gobject.option'] = option


class GObjectMeta(type):
    "Metaclass for automatically registering GObject classes"
    def __init__(cls, name, bases, dict_):
        type.__init__(cls, name, bases, dict_)
        cls._install_properties()
        cls._type_register(cls.__dict__)

    def _install_properties(cls):
        gproperties = getattr(cls, '__gproperties__', {})

        props = []
        for name, prop in cls.__dict__.items():
            if isinstance(prop, Property):  # not same as the built-in
                if name in gproperties:
                    raise ValueError
                prop.name = name
                gproperties[name] = prop.get_pspec_args()
                props.append(prop)

        if not props:
            return

        cls.__gproperties__ = gproperties

        if 'do_get_property' in cls.__dict__ or 'do_set_property' in cls.__dict__:
            for prop in props:
                if prop.fget != prop._default_getter or prop.fset != prop._default_setter:
                    raise TypeError(
                        "GObject subclass %r defines do_get/set_property"
                        " and it also uses a property with a custom setter"
                        " or getter. This is not allowed" % (
                        cls.__name__,))

        def obj_get_property(self, pspec):
            name = pspec.name.replace('-', '_')
            prop = getattr(cls, name, None)
            if prop:
                return prop.fget(self)
        cls.do_get_property = obj_get_property

        def obj_set_property(self, pspec, value):
            name = pspec.name.replace('-', '_')
            prop = getattr(cls, name, None)
            if prop:
                prop.fset(self, value)
        cls.do_set_property = obj_set_property

    def _type_register(cls, namespace):
        ## don't register the class if already registered
        if '__gtype__' in namespace:
            return

        # Do not register a new GType for the overrides, as this would sort of
        # defeat the purpose of overrides...
        if cls.__module__.startswith('gi.overrides.'):
            return

        type_register(cls, namespace.get('__gtype_name__'))

_gobject._install_metaclass(GObjectMeta)

# Deprecated naming still available for backwards compatibility.
property = Property
