# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2012 Canonical Ltd.
# Author: Martin Pitt <martin.pitt@ubuntu.com>
# Copyright (C) 2012-2013 Simon Feltman <sfeltman@src.gnome.org>
# Copyright (C) 2012 Bastian Winkler <buz@netbuz.org>
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

import functools
from collections import namedtuple

import gi.module
from gi.overrides import override
from gi.repository import GLib
from gi import _propertyhelper as propertyhelper
from gi import _signalhelper as signalhelper
from gi import _gi


GObjectModule = gi.module.get_introspection_module('GObject')

__all__ = []


from gi import _option as option
option = option

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
TYPE_VALUE = GObjectModule.Value.__gtype__
TYPE_UNICHAR = TYPE_UINT
__all__ += ['TYPE_INVALID', 'TYPE_NONE', 'TYPE_INTERFACE', 'TYPE_CHAR',
            'TYPE_UCHAR', 'TYPE_BOOLEAN', 'TYPE_INT', 'TYPE_UINT', 'TYPE_LONG',
            'TYPE_ULONG', 'TYPE_INT64', 'TYPE_UINT64', 'TYPE_ENUM', 'TYPE_FLAGS',
            'TYPE_FLOAT', 'TYPE_DOUBLE', 'TYPE_STRING', 'TYPE_POINTER',
            'TYPE_BOXED', 'TYPE_PARAM', 'TYPE_OBJECT', 'TYPE_PYOBJECT',
            'TYPE_GTYPE', 'TYPE_STRV', 'TYPE_VARIANT', 'TYPE_GSTRING',
            'TYPE_UNICHAR', 'TYPE_VALUE']


# Static types
GBoxed = _gi.GBoxed
GEnum = _gi.GEnum
GFlags = _gi.GFlags
GInterface = _gi.GInterface
GObject = _gi.GObject
GObjectWeakRef = _gi.GObjectWeakRef
GParamSpec = _gi.GParamSpec
GPointer = _gi.GPointer
GType = _gi.GType
Warning = _gi.Warning
__all__ += ['GBoxed', 'GEnum', 'GFlags', 'GInterface', 'GObject',
            'GObjectWeakRef', 'GParamSpec', 'GPointer', 'GType',
            'Warning']


features = {'generic-c-marshaller': True}
list_properties = _gi.list_properties
new = _gi.new
pygobject_version = _gi.pygobject_version
type_register = _gi.type_register
__all__ += ['features', 'list_properties', 'new',
            'pygobject_version', 'type_register']


class Value(GObjectModule.Value):
    def __init__(self, value_type=None, py_value=None):
        GObjectModule.Value.__init__(self)
        if value_type is not None:
            self.init(value_type)
            if py_value is not None:
                self.set_value(py_value)

    @property
    def __g_type(self):
        # XXX: This is the same as self.g_type, but the field marshalling
        # code is currently very slow.
        return _gi._gvalue_get_type(self)

    def set_boxed(self, boxed):
        assert self.__g_type.is_a(TYPE_BOXED)
        # Workaround the introspection marshalers inability to know
        # these methods should be marshaling boxed types. This is because
        # the type information is stored on the GValue.
        _gi._gvalue_set(self, boxed)

    def get_boxed(self):
        assert self.__g_type.is_a(TYPE_BOXED)
        return _gi._gvalue_get(self)

    def set_value(self, py_value):
        gtype = self.__g_type

        if gtype == TYPE_CHAR:
            self.set_char(py_value)
        elif gtype == TYPE_UCHAR:
            self.set_uchar(py_value)
        elif gtype == TYPE_STRING:
            if not isinstance(py_value, str) and py_value is not None:
                raise TypeError("Expected string but got %s%s" %
                                (py_value, type(py_value)))
            _gi._gvalue_set(self, py_value)
        elif gtype == TYPE_PARAM:
            self.set_param(py_value)
        elif gtype.is_a(TYPE_FLAGS):
            self.set_flags(py_value)
        elif gtype == TYPE_POINTER:
            self.set_pointer(py_value)
        elif gtype == TYPE_GTYPE:
            self.set_gtype(py_value)
        elif gtype == TYPE_VARIANT:
            self.set_variant(py_value)
        else:
            # Fall back to _gvalue_set which handles some more cases
            # like fundamentals for which a converter is registered
            try:
                _gi._gvalue_set(self, py_value)
            except TypeError:
                if gtype == TYPE_INVALID:
                    raise TypeError("GObject.Value needs to be initialized first")
                raise

    def get_value(self):
        gtype = self.__g_type

        if gtype == TYPE_CHAR:
            return self.get_char()
        elif gtype == TYPE_UCHAR:
            return self.get_uchar()
        elif gtype == TYPE_PARAM:
            return self.get_param()
        elif gtype.is_a(TYPE_ENUM):
            return self.get_enum()
        elif gtype.is_a(TYPE_FLAGS):
            return self.get_flags()
        elif gtype == TYPE_POINTER:
            return self.get_pointer()
        elif gtype == TYPE_GTYPE:
            return self.get_gtype()
        elif gtype == TYPE_VARIANT:
            # get_variant was missing annotations
            # https://gitlab.gnome.org/GNOME/glib/merge_requests/492
            return self.dup_variant()
        else:
            try:
                return _gi._gvalue_get(self)
            except TypeError:
                if gtype == TYPE_INVALID:
                    return None
                raise

    def __repr__(self):
        return '<Value (%s) %s>' % (self.__g_type.name, self.get_value())


Value = override(Value)
__all__.append('Value')


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


SignalQuery = namedtuple('SignalQuery',
                         ['signal_id',
                          'signal_name',
                          'itype',
                          'signal_flags',
                          'return_type',
                          # n_params',
                          'param_types'])


def signal_query(id_or_name, type_=None):
    if type_ is not None:
        id_or_name = signal_lookup(id_or_name, type_)

    res = GObjectModule.signal_query(id_or_name)
    assert res is not None

    if res.signal_id == 0:
        return None

    # Return a named tuple to allows indexing which is compatible with the
    # static bindings along with field like access of the gi struct.
    # Note however that the n_params was not returned from the static bindings
    # so we must skip over it.
    return SignalQuery(res.signal_id, res.signal_name, res.itype,
                       res.signal_flags, res.return_type,
                       tuple(res.param_types))


__all__.append('signal_query')


class _HandlerBlockManager(object):
    def __init__(self, obj, handler_id):
        self.obj = obj
        self.handler_id = handler_id

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        GObjectModule.signal_handler_unblock(self.obj, self.handler_id)


def signal_handler_block(obj, handler_id):
    """Blocks the signal handler from being invoked until
    handler_unblock() is called.

    :param GObject.Object obj:
        Object instance to block handlers for.
    :param int handler_id:
        Id of signal to block.
    :returns:
        A context manager which optionally can be used to
        automatically unblock the handler:

    .. code-block:: python

        with GObject.signal_handler_block(obj, id):
            pass
    """
    GObjectModule.signal_handler_block(obj, handler_id)
    return _HandlerBlockManager(obj, handler_id)


__all__.append('signal_handler_block')


def signal_parse_name(detailed_signal, itype, force_detail_quark):
    """Parse a detailed signal name into (signal_id, detail).

    :param str detailed_signal:
        Signal name which can include detail.
        For example: "notify:prop_name"
    :returns:
        Tuple of (signal_id, detail)
    :raises ValueError:
        If the given signal is unknown.
    """
    res, signal_id, detail = GObjectModule.signal_parse_name(detailed_signal, itype,
                                                             force_detail_quark)
    if res:
        return signal_id, detail
    else:
        raise ValueError('%s: unknown signal name: %s' % (itype, detailed_signal))


__all__.append('signal_parse_name')


def remove_emission_hook(obj, detailed_signal, hook_id):
    signal_id, detail = signal_parse_name(detailed_signal, obj, True)
    GObjectModule.signal_remove_emission_hook(signal_id, hook_id)


__all__.append('remove_emission_hook')


# GObject accumulators with pure Python implementations
# These return a tuple of (continue_emission, accumulation_result)

def signal_accumulator_first_wins(ihint, return_accu, handler_return, user_data=None):
    # Stop emission but return the result of the last handler
    return (False, handler_return)


__all__.append('signal_accumulator_first_wins')


def signal_accumulator_true_handled(ihint, return_accu, handler_return, user_data=None):
    # Stop emission if the last handler returns True
    return (not handler_return, handler_return)


__all__.append('signal_accumulator_true_handled')


# Statically bound signal functions which need to clobber GI (for now)

add_emission_hook = _gi.add_emission_hook
signal_new = _gi.signal_new

__all__ += ['add_emission_hook', 'signal_new']


class _FreezeNotifyManager(object):
    def __init__(self, obj):
        self.obj = obj

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        self.obj.thaw_notify()


def _signalmethod(func):
    # Function wrapper for signal functions used as instance methods.
    # This is needed when the signal functions come directly from GI.
    # (they are not already wrapped)
    @functools.wraps(func)
    def meth(*args, **kwargs):
        return func(*args, **kwargs)
    return meth


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
    interface_find_property = _unsupported_method
    interface_install_property = _unsupported_method
    interface_list_properties = _unsupported_method
    notify_by_pspec = _unsupported_method
    watch_closure = _unsupported_method

    # Make all reference management methods private but still accessible.
    _ref = GObjectModule.Object.ref
    _ref_sink = GObjectModule.Object.ref_sink
    _unref = GObjectModule.Object.unref
    _force_floating = GObjectModule.Object.force_floating

    ref = _unsupported_method
    ref_sink = _unsupported_method
    unref = _unsupported_method
    force_floating = _unsupported_method

    # The following methods are static APIs which need to leap frog the
    # gi methods until we verify the gi methods can replace them.
    get_property = _gi.GObject.get_property
    get_properties = _gi.GObject.get_properties
    set_property = _gi.GObject.set_property
    set_properties = _gi.GObject.set_properties
    bind_property = _gi.GObject.bind_property
    connect = _gi.GObject.connect
    connect_after = _gi.GObject.connect_after
    connect_object = _gi.GObject.connect_object
    connect_object_after = _gi.GObject.connect_object_after
    disconnect_by_func = _gi.GObject.disconnect_by_func
    handler_block_by_func = _gi.GObject.handler_block_by_func
    handler_unblock_by_func = _gi.GObject.handler_unblock_by_func
    emit = _gi.GObject.emit
    chain = _gi.GObject.chain
    weak_ref = _gi.GObject.weak_ref
    __copy__ = _gi.GObject.__copy__
    __deepcopy__ = _gi.GObject.__deepcopy__

    def freeze_notify(self):
        """Freezes the object's property-changed notification queue.

        :returns:
            A context manager which optionally can be used to
            automatically thaw notifications.

        This will freeze the object so that "notify" signals are blocked until
        the thaw_notify() method is called.

        .. code-block:: python

            with obj.freeze_notify():
                pass
        """
        super(Object, self).freeze_notify()
        return _FreezeNotifyManager(self)

    def connect_data(self, detailed_signal, handler, *data, **kwargs):
        """Connect a callback to the given signal with optional user data.

        :param str detailed_signal:
            A detailed signal to connect to.
        :param callable handler:
            Callback handler to connect to the signal.
        :param *data:
            Variable data which is passed through to the signal handler.
        :param GObject.ConnectFlags connect_flags:
            Flags used for connection options.
        :returns:
            A signal id which can be used with disconnect.
        """
        flags = kwargs.get('connect_flags', 0)
        if flags & GObjectModule.ConnectFlags.AFTER:
            connect_func = _gi.GObject.connect_after
        else:
            connect_func = _gi.GObject.connect

        if flags & GObjectModule.ConnectFlags.SWAPPED:
            if len(data) != 1:
                raise ValueError('Using GObject.ConnectFlags.SWAPPED requires exactly '
                                 'one argument for user data, got: %s' % [data])

            def new_handler(obj, *args):
                # Swap obj with the last element in args which will be the user
                # data passed to the connect function.
                args = list(args)
                swap = args.pop()
                args = args + [obj]
                return handler(swap, *args)
        else:
            new_handler = handler

        return connect_func(self, detailed_signal, new_handler, *data)

    #
    # Aliases
    #

    handler_block = signal_handler_block
    handler_unblock = _signalmethod(GObjectModule.signal_handler_unblock)
    disconnect = _signalmethod(GObjectModule.signal_handler_disconnect)
    handler_disconnect = _signalmethod(GObjectModule.signal_handler_disconnect)
    handler_is_connected = _signalmethod(GObjectModule.signal_handler_is_connected)
    stop_emission_by_name = _signalmethod(GObjectModule.signal_stop_emission_by_name)


Object = override(Object)
GObject = Object
__all__ += ['Object', 'GObject']


class Binding(GObjectModule.Binding):

    def unbind(self):
        # Fixed in newer glib
        if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION, GLib.MICRO_VERSION) >= (2, 57, 3):
            return super(Binding, self).unbind()

        if hasattr(self, '_unbound'):
            raise ValueError('binding has already been cleared out')
        else:
            setattr(self, '_unbound', True)
            super(Binding, self).unbind()


Binding = override(Binding)
__all__.append('Binding')


Property = propertyhelper.Property
Signal = signalhelper.Signal
SignalOverride = signalhelper.SignalOverride
__all__ += ['Property', 'Signal', 'SignalOverride', 'property']
