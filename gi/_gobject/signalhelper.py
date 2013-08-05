# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2012 Simon Feltman
#
#   gobject/signalhelper.py: GObject signal binding decorator object
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

import sys
import inspect

from . import _gobject

# Callable went away in python 3.0 and came back in 3.2.
# Use versioning to figure out when to define it, otherwise we have to deal with
# the complexity of using __builtin__ or builtin between python versions to
# check if callable exists which PyFlakes will also complain about.
if (3, 0) <= sys.version_info < (3, 2):
    def callable(fn):
        return hasattr(fn, '__call__')


class Signal(str):
    """
    Object which gives a nice API for creating and binding signals.

    Example:
    class Spam(GObject.GObject):
        velocity = 0

        @GObject.Signal
        def pushed(self):
            self.velocity += 1

        @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
        def pulled(self):
            self.velocity -= 1

        stomped = GObject.Signal('stomped', arg_types=(int,))

        @GObject.Signal
        def annotated_signal(self, a:int, b:str):
            "Python3 annotation support for parameter types.

    def on_pushed(obj):
        print(obj)

    spam = Spam()
    spam.pushed.connect(on_pushed)
    spam.pushed.emit()
    """
    class BoundSignal(str):
        """
        Temporary binding object which can be used for connecting signals
        without specifying the signal name string to connect.
        """
        def __new__(cls, name, *args, **kargs):
            return str.__new__(cls, name)

        def __init__(self, signal, gobj):
            str.__init__(self)
            self.signal = signal
            self.gobj = gobj

        def __repr__(self):
            return 'BoundSignal("%s")' % self

        def __call__(self, *args, **kargs):
            """Call the signals closure."""
            return self.signal.func(self.gobj, *args, **kargs)

        def connect(self, callback, *args, **kargs):
            """Same as GObject.GObject.connect except there is no need to specify
            the signal name."""
            return self.gobj.connect(self, callback, *args, **kargs)

        def connect_detailed(self, callback, detail, *args, **kargs):
            """Same as GObject.GObject.connect except there is no need to specify
            the signal name. In addition concats "::<detail>" to the signal name
            when connecting; for use with notifications like "notify" when a property
            changes.
            """
            return self.gobj.connect(self + '::' + detail, callback, *args, **kargs)

        def disconnect(self, handler_id):
            """Same as GObject.GObject.disconnect."""
            self.instance.disconnect(handler_id)

        def emit(self, *args, **kargs):
            """Same as GObject.GObject.emit except there is no need to specify
            the signal name."""
            return self.gobj.emit(str(self), *args, **kargs)

    def __new__(cls, name='', *args, **kargs):
        if callable(name):
            name = name.__name__
        return str.__new__(cls, name)

    def __init__(self, name='', func=None, flags=_gobject.SIGNAL_RUN_FIRST,
                 return_type=None, arg_types=None, doc='', accumulator=None, accu_data=None):
        """
        @param  name: name of signal or closure method when used as direct decorator.
        @type   name: string or callable
        @param  func: closure method.
        @type   func: callable
        @param  flags: flags specifying when to run closure
        @type   flags: GObject.SignalFlags
        @param  return_type: return type
        @type   return_type: type
        @param  arg_types: list of argument types specifying the signals function signature
        @type   arg_types: None
        @param  doc: documentation of signal object
        @type   doc: string
        @param  accumulator: accumulator method with the signature:
                func(ihint, return_accu, handler_return, accu_data) -> boolean
        @type   accumulator: function
        @param  accu_data: user data passed to the accumulator
        @type   accu_data: object
        """
        if func and not name:
            name = func.__name__
        elif callable(name):
            func = name
            name = func.__name__
        if func and not doc:
            doc = func.__doc__

        str.__init__(self)

        if func and not (return_type or arg_types):
            return_type, arg_types = get_signal_annotations(func)
        if arg_types is None:
            arg_types = tuple()

        self.func = func
        self.flags = flags
        self.return_type = return_type
        self.arg_types = arg_types
        self.__doc__ = doc
        self.accumulator = accumulator
        self.accu_data = accu_data

    def __get__(self, instance, owner=None):
        """Returns a BoundSignal when accessed on an object instance."""
        if instance is None:
            return self
        return self.BoundSignal(self, instance)

    def __call__(self, obj, *args, **kargs):
        """Allows for instantiated Signals to be used as a decorator or calling
        of the underlying signal method."""

        # If obj is a GObject, than we call this signal as a closure otherwise
        # it is used as a re-application of a decorator.
        if isinstance(obj, _gobject.GObject):
            self.func(obj, *args, **kargs)
        else:
            # If self is already an allocated name, use it otherwise create a new named
            # signal using the closure name as the name.
            if str(self):
                name = str(self)
            else:
                name = obj.__name__
            # Return a new value of this type since it is based on an immutable string.
            return type(self)(name=name, func=obj, flags=self.flags,
                              return_type=self.return_type, arg_types=self.arg_types,
                              doc=self.__doc__, accumulator=self.accumulator, accu_data=self.accu_data)

    def copy(self, newName=None):
        """Returns a renamed copy of the Signal."""
        if newName is None:
            newName = self.name
        return type(self)(name=newName, func=self.func, flags=self.flags,
                          return_type=self.return_type, arg_types=self.arg_types,
                          doc=self.__doc__, accumulator=self.accumulator, accu_data=self.accu_data)

    def get_signal_args(self):
        """Returns a tuple of: (flags, return_type, arg_types, accumulator, accu_data)"""
        return (self.flags, self.return_type, self.arg_types, self.accumulator, self.accu_data)


class SignalOverride(Signal):
    """Specialized sub-class of signal which can be used as a decorator for overriding
    existing signals on GObjects.

    Example:
    class MyWidget(Gtk.Widget):
        @GObject.SignalOverride
        def configure_event(self):
            pass
    """
    def get_signal_args(self):
        """Returns the string 'override'."""
        return 'override'


def get_signal_annotations(func):
    """Attempt pulling python 3 function annotations off of 'func' for
    use as a signals type information. Returns an ordered nested tuple
    of (return_type, (arg_type1, arg_type2, ...)). If the given function
    does not have annotations then (None, tuple()) is returned.
    """
    arg_types = tuple()
    return_type = None

    if hasattr(func, '__annotations__'):
        spec = inspect.getfullargspec(func)
        arg_types = tuple(spec.annotations[arg] for arg in spec.args
                          if arg in spec.annotations)
        if 'return' in spec.annotations:
            return_type = spec.annotations['return']

    return return_type, arg_types


def install_signals(cls):
    """Adds Signal instances on a GObject derived class into the '__gsignals__'
    dictionary to be picked up and registered as real GObject signals.
    """
    gsignals = cls.__dict__.get('__gsignals__', {})
    newsignals = {}
    for name, signal in cls.__dict__.items():
        if isinstance(signal, Signal):
            signalName = str(signal)
            # Fixup a signal which is unnamed by using the class variable name.
            # Since Signal is based on string which immutable,
            # we must copy and replace the class variable.
            if not signalName:
                signalName = name
                signal = signal.copy(name)
                setattr(cls, name, signal)
            if signalName in gsignals:
                raise ValueError('Signal "%s" has already been registered.' % name)
            newsignals[signalName] = signal
            gsignals[signalName] = signal.get_signal_args()

    cls.__gsignals__ = gsignals

    # Setup signal closures by adding the specially named
    # method to the class in the form of "do_<signal_name>".
    for name, signal in newsignals.items():
        if signal.func is not None:
            funcName = 'do_' + name.replace('-', '_')
            if not hasattr(cls, funcName):
                setattr(cls, funcName, signal.func)
