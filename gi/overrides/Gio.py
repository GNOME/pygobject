# Copyright (C) 2010 Ignacio Casal Quinteiro <icq@gnome.org>
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

import asyncio
import warnings

from contextlib import suppress

from .._ossighelper import register_sigint_fallback, get_event_loop
from ..overrides import (
    override,
    deprecated_attr,
    deprecated_init,
    wrap_list_store_equal_func,
    wrap_list_store_sort_func,
)
from .._gi import CallableInfo
from ..module import get_introspection_module
from gi import PyGIWarning

from gi.repository import GLib
from gi.repository import GObject

from typing import Callable, Generic, TypeVar, Union, overload
from collections.abc import Generator, Sequence

import sys

Gio = get_introspection_module("Gio")

__all__ = []


class ActionMap(Gio.ActionMap):
    def add_action_entries(self, entries, user_data=None):
        """The ``add_action_entries()`` method is a convenience function for creating
        multiple :class:`~gi.repository.Gio.SimpleAction` instances and adding them
        to a :class:`~gi.repository.Gio.ActionMap`.
        Each action is constructed as per one entry.

        :param list entries:
            List of entry tuples for :meth:`add_action` method. The entry tuple can
            vary in size with the following information:

            * The name of the action. Must be specified.
            * The callback to connect to the "activate" signal of the
              action. Since GLib 2.40, this can be ``None`` for stateful
              actions, in which case the default handler is used. For
              boolean-stated actions with no parameter, this is a toggle.
              For other state types (and parameter type equal to the state
              type) this will be a function that just calls change_state
              (which you should provide).
            * The type of the parameter that must be passed to the activate
              function for this action, given as a single :class:`~gi.repository.GLib.Variant` type
              string (or ``None`` for no parameter)
            * The initial state for this action, given in GLib.Variant text
              format. The state is parsed with no extra type information, so
              type tags must be added to the string if they are necessary.
              Stateless actions should give ``None`` here.
            * The callback to connect to the "change-state" signal of the
              action. All stateful actions should provide a handler here;
              stateless actions should not.

        :param user_data:
            The user data for signal connections, or ``None``
        """
        try:
            iter(entries)
        except TypeError:
            raise TypeError("entries must be iterable")

        def _process_action(
            name, activate=None, parameter_type=None, state=None, change_state=None
        ):
            if parameter_type:
                if not GLib.VariantType.string_is_valid(parameter_type):
                    msg = f"The type string '{parameter_type}' given as the parameter type for action '{name}' is not a valid GVariant type string. "
                    raise TypeError(msg)
                variant_parameter = GLib.VariantType.new(parameter_type)
            else:
                variant_parameter = None

            if state is not None:
                # stateful action
                variant_state = GLib.Variant.parse(None, state, None, None)
                action = Gio.SimpleAction.new_stateful(
                    name, variant_parameter, variant_state
                )
                if change_state is not None:
                    action.connect("change-state", change_state, user_data)
            else:
                # stateless action
                if change_state is not None:
                    msg = f"Stateless action '{name}' should give None for 'change_state', not '{change_state}'."
                    raise ValueError(msg)
                action = Gio.SimpleAction(name=name, parameter_type=variant_parameter)

            if activate is not None:
                action.connect("activate", activate, user_data)
            self.add_action(action)

        for entry in entries:
            # using inner function above since entries can leave out optional arguments
            _process_action(*entry)


ActionMap = override(ActionMap)
__all__.append("ActionMap")


class Application(Gio.Application):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._asyncio_tasks = set()

    def run(self, *args, **kwargs):
        with (
            register_sigint_fallback(self.quit),
            get_event_loop(GLib.MainContext.default()).running(self.quit),
        ):
            return Gio.Application.run(self, *args, **kwargs)

    def create_asyncio_task(self, coro):
        """Safely create an asyncio task. The application will not quit until the
        task completes. For potentially longer running tasks, you should add
        cancellation logic to abort a task when it is not needed anymore (e.g.
        cancelling it from the Gtk.Window.do_unmap event).

        Note that python will only log a raised exception if the Task is
        destroyed without the result having been collected. However, this does
        also not happen when the task is cancelled. As such, be careful to not
        cancel tasks that are already finished.

        You can deal with this by either only storing a weak reference to the
        Task, by explicitly collecting the result, or by only cancelling it if
        it is not done already.
        """
        # First create the task (in case it raises an exception)
        task = asyncio.create_task(coro)

        self.hold()

        # Store a reference to the task so that it cannot be garbage collected
        self._asyncio_tasks.add(task)

        def done_cb(task):
            nonlocal self
            self._asyncio_tasks.discard(task)
            self.release()

        task.add_done_callback(done_cb)
        return task


Application = override(Application)
__all__.append("Application")


def _warn_init(cls, instead=None):
    def new_init(self, *args, **kwargs):
        super(cls, self).__init__(*args, **kwargs)
        name = cls.__module__.rsplit(".", 1)[-1] + "." + cls.__name__
        if instead:
            warnings.warn(
                (f"{name} shouldn't be instantiated directly, use {instead} instead."),
                PyGIWarning,
                stacklevel=2,
            )
        else:
            warnings.warn(
                f"{name} shouldn't be instantiated directly.",
                PyGIWarning,
                stacklevel=2,
            )

    return new_init


@override
class VolumeMonitor(Gio.VolumeMonitor):
    # https://bugzilla.gnome.org/show_bug.cgi?id=744690
    __init__ = _warn_init(Gio.VolumeMonitor, "Gio.VolumeMonitor.get()")


__all__.append("VolumeMonitor")


@override
class DBusAnnotationInfo(Gio.DBusAnnotationInfo):
    __init__ = _warn_init(Gio.DBusAnnotationInfo)


__all__.append("DBusAnnotationInfo")


@override
class DBusArgInfo(Gio.DBusArgInfo):
    __init__ = _warn_init(Gio.DBusArgInfo)


__all__.append("DBusArgInfo")


@override
class DBusMethodInfo(Gio.DBusMethodInfo):
    __init__ = _warn_init(Gio.DBusMethodInfo)


__all__.append("DBusMethodInfo")


@override
class DBusSignalInfo(Gio.DBusSignalInfo):
    __init__ = _warn_init(Gio.DBusSignalInfo)


__all__.append("DBusSignalInfo")


@override
class DBusInterfaceInfo(Gio.DBusInterfaceInfo):
    __init__ = _warn_init(Gio.DBusInterfaceInfo)


__all__.append("DBusInterfaceInfo")


@override
class DBusNodeInfo(Gio.DBusNodeInfo):
    __init__ = _warn_init(Gio.DBusNodeInfo)


__all__.append("DBusNodeInfo")


class FileEnumerator(Gio.FileEnumerator):
    def __iter__(self):
        return self

    def __next__(self):
        file_info = self.next_file(None)

        if file_info is not None:
            return file_info
        raise StopIteration


FileEnumerator = override(FileEnumerator)
__all__.append("FileEnumerator")


class MenuItem(Gio.MenuItem):
    def set_attribute(self, attributes):
        for name, format_string, value in attributes:
            self.set_attribute_value(name, GLib.Variant(format_string, value))


MenuItem = override(MenuItem)
__all__.append("MenuItem")


class Settings(Gio.Settings):
    """Provide dictionary-like access to GLib.Settings."""

    __init__ = deprecated_init(
        Gio.Settings.__init__, arg_names=("schema", "path", "backend")
    )

    def __contains__(self, key):
        return key in self.list_keys()

    def __len__(self):
        return len(self.list_keys())

    def __iter__(self):
        yield from self.list_keys()

    def __bool__(self):
        # for "if mysettings" we don't want a dictionary-like test here, just
        # if the object isn't None
        return True

    def __getitem__(self, key):
        # get_value() aborts the program on an unknown key
        if key not in self:
            raise KeyError(f"unknown key: {key!r}")

        return self.get_value(key).unpack()

    def __setitem__(self, key, value):
        # set_value() aborts the program on an unknown key
        if key not in self:
            raise KeyError(f"unknown key: {key!r}")

        # determine type string of this key
        range = self.get_range(key)
        type_ = range.get_child_value(0).get_string()
        v = range.get_child_value(1)
        if type_ == "type":
            # v is boxed empty array, type of its elements is the allowed value type
            type_str = v.get_child_value(0).get_type_string()
            assert type_str.startswith("a")
            type_str = type_str[1:]
        elif type_ == "enum":
            # v is an array with the allowed values
            assert v.get_child_value(0).get_type_string().startswith("a")
            type_str = v.get_child_value(0).get_child_value(0).get_type_string()
            allowed = v.unpack()
            if value not in allowed:
                raise ValueError(f"value {value} is not an allowed enum ({allowed})")
        elif type_ == "range":
            tuple_ = v.get_child_value(0)
            type_str = tuple_.get_child_value(0).get_type_string()
            min_, max_ = tuple_.unpack()
            if value < min_ or value > max_:
                raise ValueError(f"value {value} not in range ({min_} - {max_})")
        else:
            raise NotImplementedError(
                "Cannot handle allowed type range class " + str(type_)
            )

        self.set_value(key, GLib.Variant(type_str, value))

    def keys(self):
        return self.list_keys()


Settings = override(Settings)
__all__.append("Settings")


class _DBusProxyMethodCall:
    """Helper class to implement DBusProxy method calls."""

    def __init__(self, dbus_proxy, method_name):
        self.dbus_proxy = dbus_proxy
        self.method_name = method_name

    def __async_result_handler(self, obj, result, user_data):
        (result_callback, error_callback, real_user_data) = user_data
        try:
            ret = obj.call_finish(result)
        except Exception:
            _etype, e = sys.exc_info()[:2]
            # return exception as value
            if error_callback:
                error_callback(obj, e, real_user_data)
            else:
                result_callback(obj, e, real_user_data)
            return

        result_callback(obj, self._unpack_result(ret), real_user_data)

    def __call__(
        self,
        *args,
        result_handler=None,
        error_handler=None,
        user_data=None,
        flags=0,
        timeout=-1,
    ):
        # the first positional argument is the signature, unless we are calling
        # a method without arguments; then signature is implied to be '()'.
        if args:
            signature = args[0]
            args = args[1:]
            if not isinstance(signature, str):
                raise TypeError(
                    f"first argument must be the method signature string: {signature!r}"
                )
        else:
            signature = "()"

        arg_variant = GLib.Variant(signature, tuple(args))

        if result_handler is not None:
            # asynchronous call
            user_data = (result_handler, error_handler, user_data)
            self.dbus_proxy.call(
                self.method_name,
                arg_variant,
                flags,
                timeout,
                None,
                self.__async_result_handler,
                user_data,
            )
        else:
            # synchronous call
            result = self.dbus_proxy.call_sync(
                self.method_name, arg_variant, flags, timeout, None
            )
            return self._unpack_result(result)
        return None

    @classmethod
    def _unpack_result(klass, result):
        """Convert a D-BUS return variant into an appropriate return value."""
        result = result.unpack()

        # to be compatible with standard Python behaviour, unbox
        # single-element tuples and return None for empty result tuples
        if len(result) == 1:
            result = result[0]
        elif len(result) == 0:
            result = None

        return result


class DBusProxy(Gio.DBusProxy):
    """Provide comfortable and pythonic method calls.

    This marshalls the method arguments into a GVariant, invokes the
    call_sync() method on the DBusProxy object, and unmarshalls the result
    GVariant back into a Python tuple.

    The first argument always needs to be the D-Bus signature tuple of the
    method call. Example:

      proxy = Gio.DBusProxy.new_sync(...)
      result = proxy.MyMethod('(is)', 42, 'hello')

    The exception are methods which take no arguments, like
    proxy.MyMethod('()'). For these you can omit the signature and just write
    proxy.MyMethod().

    Optional keyword arguments:

    - timeout: timeout for the call in milliseconds (default to D-Bus timeout)

    - flags: Combination of Gio.DBusCallFlags.*

    - result_handler: Do an asynchronous method call and invoke
         result_handler(proxy_object, result, user_data) when it finishes.

    - error_handler: If the asynchronous call raises an exception,
      error_handler(proxy_object, exception, user_data) is called when it
      finishes. If error_handler is not given, result_handler is called with
      the exception object as result instead.

    - user_data: Optional user data to pass to result_handler for
      asynchronous calls.

    Example for asynchronous calls:

      def mymethod_done(proxy, result, user_data):
          if isinstance(result, Exception):
              # handle error
          else:
              # do something with result

      proxy.MyMethod('(is)', 42, 'hello',
          result_handler=mymethod_done, user_data='data')
    """

    def __getattr__(self, name):
        if name.startswith("do_"):
            return self[name]
        return _DBusProxyMethodCall(self, name)


DBusProxy = override(DBusProxy)
__all__.append("DBusProxy")


ObjectItemType = TypeVar("ObjectItemType", bound=GObject.Object)


class ListModel(Gio.ListModel, Generic[ObjectItemType]):
    @overload
    def __getitem__(self, key: slice) -> list[ObjectItemType]: ...

    @overload
    def __getitem__(self, key: int) -> ObjectItemType: ...

    def __getitem__(self, key):
        if isinstance(key, slice):
            return [self.get_item(i) for i in range(*key.indices(len(self)))]
        if isinstance(key, int):
            if key < 0:
                key += len(self)
            if key < 0:
                raise IndexError
            ret = self.get_item(key)
            if ret is None:
                raise IndexError
            return ret
        raise TypeError

    def __contains__(self, item: ObjectItemType) -> bool:
        pytype = self.get_item_type().pytype
        if not isinstance(item, pytype):
            raise TypeError(f"Expected type {pytype.__module__}.{pytype.__name__}")
        return any(i == item for i in self)

    def __len__(self) -> int:
        return self.get_n_items()

    def __iter__(self) -> Generator[ObjectItemType, None, None]:
        for i in range(len(self)):
            yield self.get_item(i)


ListModel = override(ListModel)
__all__.append("ListModel")


class ListStore(Gio.ListStore, Generic[ObjectItemType]):
    # Describing the variadic arguments requires TypeVarTuple and unpacking syntax in type annotation:
    #   compare_func: Callable[Concatenate[ObjectItemType, ObjectItemType, *TypedVarTuple('Args')], int]
    #   *user_data: *TypedVarTuple('Args')
    # Since those are only available on Python ≥ 3.11, let's keep the arguments untyped for now.
    def sort(self, compare_func: Callable[..., int], *user_data) -> None:
        compare_func = wrap_list_store_sort_func(compare_func)
        return super().sort(compare_func, *user_data)

    def insert_sorted(
        self, item: ObjectItemType, compare_func: Callable[..., int], *user_data
    ) -> None:
        compare_func = wrap_list_store_sort_func(compare_func)
        return super().insert_sorted(item, compare_func, *user_data)

    def __delitem__(self, key: Union[int, slice]) -> None:
        if isinstance(key, slice):
            start, stop, step = key.indices(len(self))
            if step == 1:
                self.splice(start, max(stop - start, 0), [])
            elif step == -1:
                self.splice(stop + 1, max(start - stop, 0), [])
            else:
                for i in sorted(range(start, stop, step), reverse=True):
                    self.remove(i)
        elif isinstance(key, int):
            if key < 0:
                key += len(self)
            if key < 0 or key >= len(self):
                raise IndexError
            self.remove(key)
        else:
            raise TypeError

    @overload
    def __setitem__(self, key: slice, value: Sequence[ObjectItemType]) -> None: ...

    @overload
    def __setitem__(self, key: int, value: ObjectItemType) -> None: ...

    def __setitem__(self, key, value):
        if isinstance(key, slice):
            pytype = self.get_item_type().pytype
            valuelist = []
            for v in value:
                if not isinstance(v, pytype):
                    raise TypeError(
                        f"Expected type {pytype.__module__}.{pytype.__name__}"
                    )
                valuelist.append(v)

            start, stop, step = key.indices(len(self))
            if step == 1:
                self.splice(start, max(stop - start, 0), valuelist)
            else:
                indices = list(range(start, stop, step))
                if len(indices) != len(valuelist):
                    raise ValueError

                if step == -1:
                    self.splice(stop + 1, max(start - stop, 0), valuelist[::-1])
                else:
                    for i, v in zip(indices, valuelist):
                        self.splice(i, 1, [v])
        elif isinstance(key, int):
            if key < 0:
                key += len(self)
            if key < 0 or key >= len(self):
                raise IndexError

            pytype = self.get_item_type().pytype
            if not isinstance(value, pytype):
                raise TypeError(f"Expected type {pytype.__module__}.{pytype.__name__}")

            self.splice(key, 1, [value])
        else:
            raise TypeError

    def find_with_equal_func(self, item, equal_func, *user_data):
        # find_with_equal_func() is not suited for language bindings,
        # see: https://gitlab.gnome.org/GNOME/glib/-/issues/2447.
        # Use find_with_equal_func_full() instead.
        equal_func = wrap_list_store_equal_func(equal_func)
        return self.find_with_equal_func_full(item, equal_func, *user_data)


ListStore = override(ListStore)
__all__.append("ListStore")


class DataInputStream(Gio.DataInputStream):
    def __iter__(self):
        return self

    def __next__(self):
        line = self.read_line_utf8(None)[0]

        if line is not None:
            return line
        raise StopIteration


DataInputStream = override(DataInputStream)
__all__.append("DataInputStream")


class File(Gio.File):
    def __fspath__(self):
        path = self.peek_path()
        # A file isn't guaranteed to have a path associated and returning
        # `None` here will result in a `TypeError` trying to subscribe to it.
        if path is None or path == "":
            raise TypeError("File has no associated path.")
        return path


File = override(File)
__all__.append("File")


GioPlatform = None

with suppress(ImportError):
    from gi.repository import GioUnix as GioPlatform

if not GioPlatform:
    with suppress(ImportError):
        from gi.repository import GioWin32 as GioPlatform

if GioPlatform:
    # Add support for using platform-specific Gio symbols.
    gio_globals = globals()

    platform_name = f"{GioPlatform._namespace[len(Gio._namespace) :]}"
    platform_name_lower = platform_name.lower()

    for attr in dir(GioPlatform):
        if attr.startswith("_"):
            continue

        original_attr = getattr(GioPlatform, attr)
        wrapper_attr = attr

        if isinstance(
            original_attr, CallableInfo
        ) and original_attr.get_symbol().startswith(f"g_{platform_name_lower}_"):
            wrapper_attr = f"{platform_name_lower}_{attr}"
        else:
            try:
                gtype = getattr(original_attr, "__gtype__")
                if gtype.name.startswith(f"G{platform_name}"):
                    wrapper_attr = f"{platform_name}{attr}"
            except AttributeError:
                pass

        if wrapper_attr == attr and hasattr(Gio, wrapper_attr):
            try:
                name = original_attr.__name__[0]
            except (AttributeError, IndexError):
                name = original_attr

            # Fallback if we don't have the original name.
            if name.islower():
                wrapper_attr = f"{platform_name_lower}_{attr}"
            elif "_" in name:
                wrapper_attr = f"{platform_name.upper()}_{attr}"
            elif name:
                wrapper_attr = f"{platform_name}{attr}"

        if (
            wrapper_attr in __all__
            or wrapper_attr in gio_globals
            or hasattr(Gio, wrapper_attr)
        ):
            continue

        gio_globals[wrapper_attr] = original_attr
        deprecated_attr(
            Gio._namespace, wrapper_attr, f"{GioPlatform._namespace}.{attr}"
        )
        __all__.append(wrapper_attr)
