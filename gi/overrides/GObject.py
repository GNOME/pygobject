# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2012 Canonical Ltd.
# Author: Martin Pitt <martin.pitt@ubuntu.com>
# Copyright (C) 2012 Simon Feltman <sfeltman@src.gnome.org>
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

import sys
from collections import namedtuple

import gi.overrides
import gi.module
from gi.overrides import override
from gi.repository import GLib

from gi._gobject import _gobject
from gi._gobject import propertyhelper
from gi._gobject import signalhelper

GObjectModule = gi.module.get_introspection_module('GObject')

__all__ = []


from gi._glib import option
sys.modules['gi._gobject.option'] = option


# API aliases for backwards compatibility
for name in ['markup_escape_text', 'get_application_name',
             'set_application_name', 'get_prgname', 'set_prgname',
             'main_depth', 'filename_display_basename',
             'filename_display_name', 'filename_from_utf8',
             'uri_list_extract_uris',
             'MainLoop', 'MainContext', 'main_context_default',
             'source_remove', 'Source', 'Idle', 'Timeout', 'PollFD',
             'idle_add', 'timeout_add', 'timeout_add_seconds',
             'io_add_watch', 'child_watch_add', 'get_current_time',
             'spawn_async']:
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
             'IO_FLAG_SET_MASK',
             'SPAWN_LEAVE_DESCRIPTORS_OPEN', 'SPAWN_DO_NOT_REAP_CHILD',
             'SPAWN_SEARCH_PATH', 'SPAWN_STDOUT_TO_DEV_NULL',
             'SPAWN_STDERR_TO_DEV_NULL', 'SPAWN_CHILD_INHERITS_STDIN',
             'SPAWN_FILE_AND_ARGV_ZERO',
             'OPTION_FLAG_HIDDEN', 'OPTION_FLAG_IN_MAIN', 'OPTION_FLAG_REVERSE',
             'OPTION_FLAG_NO_ARG', 'OPTION_FLAG_FILENAME', 'OPTION_FLAG_OPTIONAL_ARG',
             'OPTION_FLAG_NOALIAS', 'OPTION_ERROR_UNKNOWN_OPTION',
             'OPTION_ERROR_BAD_VALUE', 'OPTION_ERROR_FAILED', 'OPTION_REMAINING',
             'glib_version']:
    globals()[name] = getattr(GLib, name)
    __all__.append(name)


G_MININT8 = GLib.MININT8
G_MAXINT8 = GLib.MAXINT8
G_MAXUINT8 = GLib.MAXUINT8
G_MININT16 = GLib.MININT16
G_MAXINT16 = GLib.MAXINT16
G_MAXUINT16 = GLib.MAXUINT16
G_MININT32 = GLib.MININT32
G_MAXINT32 = GLib.MAXINT32
G_MAXUINT32 = GLib.MAXUINT32
G_MININT64 = GLib.MININT64
G_MAXINT64 = GLib.MAXINT64
G_MAXUINT64 = GLib.MAXUINT64
__all__ += ['G_MININT8', 'G_MAXINT8', 'G_MAXUINT8', 'G_MININT16',
            'G_MAXINT16', 'G_MAXUINT16', 'G_MININT32', 'G_MAXINT32',
            'G_MAXUINT32', 'G_MININT64', 'G_MAXINT64', 'G_MAXUINT64']

# these are not currently exported in GLib gir, presumably because they are
# platform dependent; so get them from our static bindings
for name in ['G_MINFLOAT', 'G_MAXFLOAT', 'G_MINDOUBLE', 'G_MAXDOUBLE',
             'G_MINSHORT', 'G_MAXSHORT', 'G_MAXUSHORT', 'G_MININT', 'G_MAXINT',
             'G_MAXUINT', 'G_MINLONG', 'G_MAXLONG', 'G_MAXULONG', 'G_MAXSIZE',
             'G_MINSSIZE', 'G_MAXSSIZE', 'G_MINOFFSET', 'G_MAXOFFSET']:
    globals()[name] = getattr(_gobject, name)
    __all__.append(name)


TYPE_INVALID = GObjectModule.type_from_name('invalid')
TYPE_NONE = GObjectModule.type_from_name('void')
TYPE_INTERFACE = GObjectModule.type_from_name('GInterface')
TYPE_CHAR = GObjectModule.type_from_name('gchar')
TYPE_UCHAR = GObjectModule.type_from_name('guchar')
TYPE_BOOLEAN = GObjectModule.type_from_name('gboolean')
TYPE_INT = GObjectModule.type_from_name('gint')
TYPE_UINT = GObjectModule.type_from_name('guint')
TYPE_LONG = GObjectModule.type_from_name('glong')
TYPE_ULONG = GObjectModule.type_from_name('gulong')
TYPE_INT64 = GObjectModule.type_from_name('gint64')
TYPE_UINT64 = GObjectModule.type_from_name('guint64')
TYPE_ENUM = GObjectModule.type_from_name('GEnum')
TYPE_FLAGS = GObjectModule.type_from_name('GFlags')
TYPE_FLOAT = GObjectModule.type_from_name('gfloat')
TYPE_DOUBLE = GObjectModule.type_from_name('gdouble')
TYPE_STRING = GObjectModule.type_from_name('gchararray')
TYPE_POINTER = GObjectModule.type_from_name('gpointer')
TYPE_BOXED = GObjectModule.type_from_name('GBoxed')
TYPE_PARAM = GObjectModule.type_from_name('GParam')
TYPE_OBJECT = GObjectModule.type_from_name('GObject')
TYPE_PYOBJECT = GObjectModule.type_from_name('PyObject')
TYPE_GTYPE = GObjectModule.type_from_name('GType')
TYPE_STRV = GObjectModule.type_from_name('GStrv')
TYPE_VARIANT = GObjectModule.type_from_name('GVariant')
TYPE_GSTRING = GObjectModule.type_from_name('GString')
TYPE_UNICHAR = TYPE_UINT
__all__ += ['TYPE_INVALID', 'TYPE_NONE', 'TYPE_INTERFACE', 'TYPE_CHAR',
            'TYPE_UCHAR', 'TYPE_BOOLEAN', 'TYPE_INT', 'TYPE_UINT', 'TYPE_LONG',
            'TYPE_ULONG', 'TYPE_INT64', 'TYPE_UINT64', 'TYPE_ENUM', 'TYPE_FLAGS',
            'TYPE_FLOAT', 'TYPE_DOUBLE', 'TYPE_STRING', 'TYPE_POINTER',
            'TYPE_BOXED', 'TYPE_PARAM', 'TYPE_OBJECT', 'TYPE_PYOBJECT',
            'TYPE_GTYPE', 'TYPE_STRV', 'TYPE_VARIANT', 'TYPE_GSTRING', 'TYPE_UNICHAR']


# Deprecated, use GLib directly
Pid = GLib.Pid
GError = GLib.GError
OptionGroup = GLib.OptionGroup
OptionContext = GLib.OptionContext
__all__ += ['Pid', 'GError', 'OptionGroup', 'OptionContext']


# Deprecated, use: GObject.ParamFlags.* directly
PARAM_CONSTRUCT = GObjectModule.ParamFlags.CONSTRUCT
PARAM_CONSTRUCT_ONLY = GObjectModule.ParamFlags.CONSTRUCT_ONLY
PARAM_LAX_VALIDATION = GObjectModule.ParamFlags.LAX_VALIDATION
PARAM_READABLE = GObjectModule.ParamFlags.READABLE
PARAM_WRITABLE = GObjectModule.ParamFlags.WRITABLE
# PARAM_READWRITE should come from the gi module but cannot due to:
# https://bugzilla.gnome.org/show_bug.cgi?id=687615
PARAM_READWRITE = PARAM_READABLE | PARAM_WRITABLE
__all__ += ['PARAM_CONSTRUCT', 'PARAM_CONSTRUCT_ONLY', 'PARAM_LAX_VALIDATION',
            'PARAM_READABLE', 'PARAM_WRITABLE', 'PARAM_READWRITE']


# Deprecated, use: GObject.SignalFlags.* directly
SIGNAL_ACTION = GObjectModule.SignalFlags.ACTION
SIGNAL_DETAILED = GObjectModule.SignalFlags.DETAILED
SIGNAL_NO_HOOKS = GObjectModule.SignalFlags.NO_HOOKS
SIGNAL_NO_RECURSE = GObjectModule.SignalFlags.NO_RECURSE
SIGNAL_RUN_CLEANUP = GObjectModule.SignalFlags.RUN_CLEANUP
SIGNAL_RUN_FIRST = GObjectModule.SignalFlags.RUN_FIRST
SIGNAL_RUN_LAST = GObjectModule.SignalFlags.RUN_LAST
__all__ += ['SIGNAL_ACTION', 'SIGNAL_DETAILED', 'SIGNAL_NO_HOOKS',
            'SIGNAL_NO_RECURSE', 'SIGNAL_RUN_CLEANUP', 'SIGNAL_RUN_FIRST',
            'SIGNAL_RUN_LAST']


# Static types
GBoxed = _gobject.GBoxed
GEnum = _gobject.GEnum
GFlags = _gobject.GFlags
GInterface = _gobject.GInterface
GObject = _gobject.GObject
GObjectWeakRef = _gobject.GObjectWeakRef
GParamSpec = _gobject.GParamSpec
GPointer = _gobject.GPointer
GType = _gobject.GType
Warning = _gobject.Warning
__all__ += ['GBoxed', 'GEnum', 'GFlags', 'GInterface', 'GObject',
            'GObjectWeakRef', 'GParamSpec', 'GPointer', 'GType',
            'Warning']


add_emission_hook = _gobject.add_emission_hook
features = _gobject.features
list_properties = _gobject.list_properties
new = _gobject.new
pygobject_version = _gobject.pygobject_version
remove_emission_hook = _gobject.remove_emission_hook
signal_accumulator_true_handled = _gobject.signal_accumulator_true_handled
signal_new = _gobject.signal_new
threads_init = _gobject.threads_init
type_register = _gobject.type_register
__all__ += ['add_emission_hook', 'features', 'list_properties',
            'new', 'pygobject_version', 'remove_emission_hook',
            'signal_accumulator_true_handled',
            'signal_new', 'threads_init', 'type_register']


def type_from_name(name):
    type_ = GObjectModule.type_from_name(name)
    if type_ == TYPE_INVALID:
        raise RuntimeError('unknown type name: %s' % name)
    return type_

__all__.append('type_from_name')


def type_parent(type_):
    parent = GObjectModule.type_parent(type_)
    if parent == TYPE_INVALID:
        raise RuntimeError('no parent for type')
    return parent

__all__.append('type_parent')


def _validate_type_for_signal_method(type_):
    if hasattr(type_, '__gtype__'):
        type_ = type_.__gtype__
    if not type_.is_instantiatable() and not type_.is_interface():
        raise TypeError('type must be instantiable or an interface, got %s' % type_)


def signal_list_ids(type_):
    _validate_type_for_signal_method(type_)
    return GObjectModule.signal_list_ids(type_)

__all__.append('signal_list_ids')


def signal_list_names(type_):
    ids = signal_list_ids(type_)
    return tuple(GObjectModule.signal_name(i) for i in ids)

__all__.append('signal_list_names')


def signal_lookup(name, type_):
    _validate_type_for_signal_method(type_)
    return GObjectModule.signal_lookup(name, type_)

__all__.append('signal_lookup')


def signal_query(id_or_name, type_=None):
    SignalQuery = namedtuple('SignalQuery',
                             ['signal_id',
                              'signal_name',
                              'itype',
                              'signal_flags',
                              'return_type',
                              # n_params',
                              'param_types'])

    # signal_query needs to use a static method until the following bugs are fixed:
    # https://bugzilla.gnome.org/show_bug.cgi?id=687550
    # https://bugzilla.gnome.org/show_bug.cgi?id=687545
    # https://bugzilla.gnome.org/show_bug.cgi?id=687541
    if type_ is not None:
        id_or_name = signal_lookup(id_or_name, type_)

    res = _gobject.signal_query(id_or_name)
    if res is None:
        return None

    # Return a named tuple which allows indexing like the static bindings
    # along with field like access of the gi struct.
    # Note however that the n_params was not returned from the static bindings.
    return SignalQuery(*res)

__all__.append('signal_query')


class _HandlerBlockManager(object):
    def __init__(self, obj, handler_id):
        self.obj = obj
        self.handler_id = handler_id

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        _gobject.GObject.handler_unblock(self.obj, self.handler_id)


class _FreezeNotifyManager(object):
    def __init__(self, obj):
        self.obj = obj

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        self.obj.thaw_notify()


class Object(GObjectModule.Object):
    def _unsupported_method(self, *args, **kargs):
        raise RuntimeError('This method is currently unsupported.')

    def _unsupported_data_method(self, *args, **kargs):
        raise RuntimeError('Data access methods are unsupported. '
                           'Use normal Python attributes instead')

    # Generic data methods are not needed in python as it can be handled
    # with standard attribute access: https://bugzilla.gnome.org/show_bug.cgi?id=641944
    get_data = _unsupported_data_method
    get_qdata = _unsupported_data_method
    set_data = _unsupported_data_method
    steal_data = _unsupported_data_method
    steal_qdata = _unsupported_data_method
    replace_data = _unsupported_data_method
    replace_qdata = _unsupported_data_method

    # The following methods as unsupported until we verify
    # they work as gi methods.
    bind_property_full = _unsupported_method
    compat_control = _unsupported_method
    force_floating = _unsupported_method
    interface_find_property = _unsupported_method
    interface_install_property = _unsupported_method
    interface_list_properties = _unsupported_method
    is_floating = _unsupported_method
    notify_by_pspec = _unsupported_method
    ref = _unsupported_method
    ref_count = _unsupported_method
    ref_sink = _unsupported_method
    run_dispose = _unsupported_method
    unref = _unsupported_method
    watch_closure = _unsupported_method

    # The following methods are static APIs which need to leap frog the
    # gi methods until we verify the gi methods can replace them.
    get_property = _gobject.GObject.get_property
    get_properties = _gobject.GObject.get_properties
    set_property = _gobject.GObject.set_property
    set_properties = _gobject.GObject.set_properties
    bind_property = _gobject.GObject.bind_property
    connect = _gobject.GObject.connect
    connect_after = _gobject.GObject.connect_after
    connect_object = _gobject.GObject.connect_object
    connect_object_after = _gobject.GObject.connect_object_after
    disconnect = _gobject.GObject.disconnect
    disconnect_by_func = _gobject.GObject.disconnect_by_func
    handler_disconnect = _gobject.GObject.handler_disconnect
    handler_is_connected = _gobject.GObject.handler_is_connected
    handler_block_by_func = _gobject.GObject.handler_block_by_func
    handler_unblock_by_func = _gobject.GObject.handler_unblock_by_func
    emit = _gobject.GObject.emit
    emit_stop_by_name = _gobject.GObject.emit_stop_by_name
    stop_emission = _gobject.GObject.stop_emission
    chain = _gobject.GObject.chain
    weak_ref = _gobject.GObject.weak_ref
    __copy__ = _gobject.GObject.__copy__
    __deepcopy__ = _gobject.GObject.__deepcopy__

    def handler_block(self, handler_id):
        """Blocks the signal handler from being invoked until handler_unblock() is called.

        Returns a context manager which optionally can be used to
        automatically unblock the handler:

        >>> with obj.handler_block(id):
        >>>    pass
        """

        # Note Object.handler_block is a static method specific to pygobject and not
        # found in introspection. We need to continue using the static method
        # until we figure out a technique to call the global signal_handler_block.
        # But this requires a gpointer to the Object which we currently don't have
        # access to in python.
        _gobject.GObject.handler_block(self, handler_id)
        return _HandlerBlockManager(self, handler_id)

    def freeze_notify(self):
        """Freezes the object's property-changed notification queue.

        This will freeze the object so that "notify" signals are blocked until
        the thaw_notify() method is called.

        Returns a context manager which optionally can be used to
        automatically thaw notifications:

        >>> with obj.freeze_notify():
        >>>     pass
        """
        super(Object, self).freeze_notify()
        return _FreezeNotifyManager(self)


Object = override(Object)
GObject = Object
__all__ += ['Object', 'GObject']


Property = propertyhelper.Property
Signal = signalhelper.Signal
SignalOverride = signalhelper.SignalOverride
# Deprecated naming "property" available for backwards compatibility.
# Keep this at the end of the file to avoid clobbering the builtin.
property = Property
__all__ += ['Property', 'Signal', 'SignalOverride', 'property']
