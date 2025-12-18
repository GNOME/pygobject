# Copyright (C) 2010 Tomeu Vizoso <tomeu.vizoso@collabora.co.uk>
# Copyright (C) 2011, 2012 Canonical Ltd.
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

from __future__ import annotations

import contextlib
import warnings
import sys
import socket
import typing

from .._ossighelper import register_sigint_fallback, get_event_loop
from ..module import (
    get_fallback_platform_specific_module_symbol,
    get_platform_specific_module,
)
from .._gi import (
    variant_type_from_string,
    source_new,
    source_set_callback,
    io_channel_read,
    main_context_query,
    PyGIWarning,
)
from ..overrides import override, deprecated, deprecated_attr
from gi import PyGIDeprecationWarning, version_info

# Typing relies on https://github.com/pygobject/pygobject-stubs.
if typing.TYPE_CHECKING:
    # Import stubs for type checking this file.
    # FIXME: For IDE typing support, you need to copy
    # pygobject-stubs/src/gi-stubs/repository/*.pyi to gi/repository/
    from gi.repository import GLib
    from typing_extensions import Self

    # Type annotations cannot have `GLib.` prefix because they are copied into
    # GLib stubs module which cannot refer to itself. Use type aliases.
    IOChannel = GLib.IOChannel
    IOCondition = GLib.IOCondition
    PollFD = GLib.PollFD
else:
    from gi.module import get_introspection_module

    GLib = get_introspection_module("GLib")

__all__ = []

# Types and functions still needed from static bindings
from gi import _gi
from gi._error import GError

Error = GError
Pid = _gi.Pid
spawn_async = _gi.spawn_async


def threads_init():
    warnings.warn(
        "Since version 3.11, calling threads_init is no longer needed. "
        "See: https://pygobject.gnome.org/guide/threading.html#threads-faq",
        PyGIDeprecationWarning,
        stacklevel=2,
    )


def gerror_matches(self, domain: str | int, code: int) -> bool:
    # Handle cases where self.domain was set to an integer for compatibility
    # with the introspected GLib.Error.
    if isinstance(self.domain, str):
        self_domain_quark = GLib.quark_from_string(self.domain)
    else:
        self_domain_quark = self.domain
    return (self_domain_quark, self.code) == (domain, code)


def gerror_new_literal(domain: int, message: str, code: int) -> GError:
    domain_quark = GLib.quark_to_string(domain)
    return GError(message, domain_quark, code)


# Monkey patch methods that rely on GLib introspection to be loaded at runtime.
Error.__name__ = "Error"
Error.__module__ = "gi.repository.GLib"
Error.__gtype__ = GLib.Error.__gtype__
Error.matches = gerror_matches
Error.new_literal = staticmethod(gerror_new_literal)


__all__ += [
    "Error",
    "GError",
    "Pid",
    "spawn_async",
    "threads_init",
]


class _VariantCreator:
    _LEAF_CONSTRUCTORS = {
        "b": GLib.Variant.new_boolean,
        "y": GLib.Variant.new_byte,
        "n": GLib.Variant.new_int16,
        "q": GLib.Variant.new_uint16,
        "i": GLib.Variant.new_int32,
        "u": GLib.Variant.new_uint32,
        "x": GLib.Variant.new_int64,
        "t": GLib.Variant.new_uint64,
        "h": GLib.Variant.new_handle,
        "d": GLib.Variant.new_double,
        "s": GLib.Variant.new_string,
        "o": GLib.Variant.new_object_path,
        "g": GLib.Variant.new_signature,
        "v": GLib.Variant.new_variant,
    }

    def _create(self, format, value):
        """Create a GVariant object from given format and a value that matches
        the format.

        This method recursively calls itself for complex structures (arrays,
        dictionaries, boxed).

        Returns the generated GVariant.

        If value is None it will generate an empty GVariant container type.
        """
        gvtype = GLib.VariantType(format)
        if format in self._LEAF_CONSTRUCTORS:
            return self._LEAF_CONSTRUCTORS[format](value)

        # Since we discarded all leaf types, this must be a container
        builder = GLib.VariantBuilder.new(gvtype)
        if value is None:
            return builder.end()

        if gvtype.is_maybe():
            builder.add_value(self._create(gvtype.element().dup_string(), value))
            return builder.end()

        try:
            iter(value)
        except TypeError:
            raise TypeError(
                f"Could not create array, tuple or dictionary entry from non iterable value {format} {value}"
            )

        if gvtype.is_tuple() and gvtype.n_items() != len(value):
            raise TypeError(
                f"Tuple mismatches value's number of elements {format} {value}"
            )
        if gvtype.is_dict_entry() and len(value) != 2:
            raise TypeError(
                f"Dictionary entries must have two elements {format} {value}"
            )

        if gvtype.is_array():
            element_type = gvtype.element().dup_string()
            if isinstance(value, dict):
                value = value.items()
            for i in value:
                builder.add_value(self._create(element_type, i))
        else:
            remainer_format = format[1:]
            for i in value:
                dup = variant_type_from_string(remainer_format).dup_string()
                builder.add_value(self._create(dup, i))
                remainer_format = remainer_format[len(dup) :]

        return builder.end()


class Variant(GLib.Variant):
    @staticmethod
    def __new__(cls: type[Self], format_string: str, value: typing.Any) -> Self:
        """Create a GVariant from a native Python object.

        format_string is a standard GVariant type signature, value is a Python
        object whose structure has to match the signature.

        Examples:
          GLib.Variant('i', 1)
          GLib.Variant('(is)', (1, 'hello'))
          GLib.Variant('(asa{sv})', ([], {'foo': GLib.Variant('b', True),
                                          'bar': GLib.Variant('i', 2)}))

        """
        if not GLib.VariantType.string_is_valid(format_string):
            raise TypeError("Invalid GVariant format string '%s'", format_string)
        creator = _VariantCreator()
        v = creator._create(format_string, value)
        v.format_string = format_string
        return v

    @staticmethod
    def new_tuple(*elements: Variant) -> Variant:  # type: ignore[override]
        return GLib.Variant.new_tuple(elements)  # type: ignore[arg-type, return-value]

    def __del__(self):
        with contextlib.suppress(ImportError):
            # Calling unref will cause gi and gi.repository.GLib to be
            # imported. However, if the program is exiting, then these
            # modules have likely been removed from sys.modules and will
            # raise an exception. Assume that's the case for ImportError
            # and ignore the exception since everything will be cleaned
            # up, anyway.
            self.unref()

    def __str__(self) -> str:
        return self.print_(True)

    def __repr__(self) -> str:
        if hasattr(self, "format_string"):
            f = self.format_string
        else:
            f = self.get_type_string()
        return f"GLib.Variant('{f}', {self.print_(False)})"

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Variant):
            return False
        try:
            return self.equal(other)
        except TypeError:
            return False

    def __ne__(self, other: object) -> bool:
        if not isinstance(other, Variant):
            return True
        try:
            return not self.equal(other)
        except TypeError:
            return True

    def __hash__(self) -> int:
        # We're not using just hash(self.unpack()) because otherwise we'll have
        # hash collisions between the same content in different variant types,
        # which will cause a performance issue in set/dict/etc.
        return hash((self.get_type_string(), self.unpack()))

    def unpack(self) -> typing.Any:
        """Decompose a GVariant into a native Python object."""
        type_string = self.get_type_string()

        # simple values
        match type_string:
            case "b":
                return self.get_boolean()
            case "y":
                return self.get_byte()
            case "n":
                return self.get_int16()
            case "q":
                return self.get_uint16()
            case "i":
                return self.get_int32()
            case "u":
                return self.get_uint32()
            case "x":
                return self.get_int64()
            case "t":
                return self.get_uint64()
            case "h":
                return self.get_handle()
            case "d":
                return self.get_double()
            case "s":
                return self.get_string()
            case "o":
                return self.get_string()  # object path
            case "g":
                return self.get_string()  # signature

        # tuple
        if type_string.startswith("("):
            return tuple(
                self.get_child_value(i).unpack() for i in range(self.n_children())
            )

        # dictionary
        if type_string.startswith("a{"):
            res: dict[bool | float | str, typing.Any] = {}
            for i in range(self.n_children()):
                v = self.get_child_value(i)
                res[v.get_child_value(0).unpack()] = v.get_child_value(1).unpack()
            return res

        # array
        if type_string.startswith("a"):
            return [self.get_child_value(i).unpack() for i in range(self.n_children())]

        # variant (just unbox transparently)
        if type_string.startswith("v"):
            return self.get_variant().unpack()

        # maybe
        if type_string.startswith("m"):
            if not self.n_children():
                return None
            return self.get_child_value(0).unpack()

        raise NotImplementedError("unsupported GVariant type " + type_string)

    @classmethod
    def split_signature(klass, signature):
        """Return a list of the element signatures of the topmost signature tuple.

        If the signature is not a tuple, it returns one element with the entire
        signature. If the signature is an empty tuple, the result is [].

        This is useful for e. g. iterating over method parameters which are
        passed as a single Variant.
        """
        if signature == "()":
            return []

        if not signature.startswith("("):
            return [signature]

        result = []
        head = ""
        tail = signature[1:-1]  # eat the surrounding ()
        while tail:
            c = tail[0]
            head += c
            tail = tail[1:]

            if c in ("m", "a"):
                # prefixes, keep collecting
                continue
            if c in ("(", "{"):
                # consume until corresponding )/}
                level = 1
                up = c
                down = ")" if up == "(" else "}"
                while level > 0:
                    c = tail[0]
                    head += c
                    tail = tail[1:]
                    if c == up:
                        level += 1
                    elif c == down:
                        level -= 1

            # otherwise we have a simple type
            result.append(head)
            head = ""

        return result

    #
    # Pythonic iterators
    #

    def __len__(self) -> int:
        if self.get_type_string() in ["s", "o", "g"]:
            return len(self.get_string())
        # Array, dict, tuple
        if self.get_type_string().startswith("a") or self.get_type_string().startswith(
            "("
        ):
            return self.n_children()
        raise TypeError(
            f"GVariant type {self.get_type_string()} does not have a length"
        )

    def __getitem__(self, key: str | int) -> typing.Any:
        # dict
        if self.get_type_string().startswith("a{"):
            if isinstance(key, str):
                val = self.lookup_value(key, variant_type_from_string("*"))
                if val is None:
                    raise KeyError(key)
                return val.unpack()
            # lookup_value() only works for string keys, which is certainly
            # the common case; we have to do painful iteration for other
            # key types
            for i in range(self.n_children()):
                v = self.get_child_value(i)
                if v.get_child_value(0).unpack() == key:
                    return v.get_child_value(1).unpack()
            raise KeyError(key)

        if not isinstance(key, int):
            raise ValueError("Key must be an integer for arrays, tuples and strings")

        # array/tuple
        if self.get_type_string().startswith("a") or self.get_type_string().startswith(
            "("
        ):
            if key < 0:
                key = self.n_children() + key
            if key < 0 or key >= self.n_children():
                raise IndexError("list index out of range")
            return self.get_child_value(key).unpack()

        # string
        if self.get_type_string() in ["s", "o", "g"]:
            return self.get_string().__getitem__(key)

        raise TypeError(f"GVariant type {self.get_type_string()} is not a container")

    #
    # Pythonic bool operations
    #

    def __nonzero__(self):
        return self.__bool__()

    def __bool__(self) -> bool:
        if self.get_type_string() in ["y", "n", "q", "i", "u", "x", "t", "h", "d"]:
            return self.unpack() != 0
        if self.get_type_string() in ["b"]:
            return self.get_boolean()
        if self.get_type_string() in ["s", "o", "g"]:
            return len(self.get_string()) != 0
        # Array, dict, tuple
        if self.get_type_string().startswith("a") or self.get_type_string().startswith(
            "("
        ):
            return self.n_children() != 0
        # unpack works recursively, hence bool also works recursively
        return bool(self.unpack())

    def keys(self) -> typing.Iterable[bool | float | str]:
        if not self.get_type_string().startswith("a{"):
            raise TypeError(
                f"GVariant type {self.get_type_string()} is not a dictionary"
            )

        res = []
        for i in range(self.n_children()):
            v = self.get_child_value(i)
            res.append(v.get_child_value(0).unpack())
        return res

    def get_string(self) -> str:
        value: str
        value, _length = super().get_string()  # type: ignore[misc]
        return value


__all__.append("Variant")


def markup_escape_text(text: str | bytes, length: int = -1) -> str:
    if isinstance(text, bytes):
        return GLib.markup_escape_text(text.decode("UTF-8"), length)
    return GLib.markup_escape_text(text, length)


__all__.append("markup_escape_text")


# backwards compatible names from old static bindings
for n in [
    "DESKTOP",
    "DOCUMENTS",
    "DOWNLOAD",
    "MUSIC",
    "PICTURES",
    "PUBLIC_SHARE",
    "TEMPLATES",
    "VIDEOS",
]:
    attr = "USER_DIRECTORY_" + n
    deprecated_attr("GLib", attr, "GLib.UserDirectory.DIRECTORY_" + n)
    globals()[attr] = getattr(GLib.UserDirectory, "DIRECTORY_" + n)
    __all__.append(attr)

for n in ["ERR", "HUP", "IN", "NVAL", "OUT", "PRI"]:
    globals()["IO_" + n] = getattr(GLib.IOCondition, n)
    __all__.append("IO_" + n)

for n in [
    "APPEND",
    "GET_MASK",
    "IS_READABLE",
    "IS_SEEKABLE",
    "MASK",
    "NONBLOCK",
    "SET_MASK",
]:
    attr = "IO_FLAG_" + n
    deprecated_attr("GLib", attr, "GLib.IOFlags." + n)
    globals()[attr] = getattr(GLib.IOFlags, n)
    __all__.append(attr)

# spelling for the win
IO_FLAG_IS_WRITEABLE = GLib.IOFlags.IS_WRITABLE
deprecated_attr("GLib", "IO_FLAG_IS_WRITEABLE", "GLib.IOFlags.IS_WRITABLE")
__all__.append("IO_FLAG_IS_WRITEABLE")

for n in ["AGAIN", "EOF", "ERROR", "NORMAL"]:
    attr = "IO_STATUS_" + n
    globals()[attr] = getattr(GLib.IOStatus, n)
    deprecated_attr("GLib", attr, "GLib.IOStatus." + n)
    __all__.append(attr)

for n in [
    "CHILD_INHERITS_STDIN",
    "DO_NOT_REAP_CHILD",
    "FILE_AND_ARGV_ZERO",
    "LEAVE_DESCRIPTORS_OPEN",
    "SEARCH_PATH",
    "STDERR_TO_DEV_NULL",
    "STDOUT_TO_DEV_NULL",
]:
    attr = "SPAWN_" + n
    globals()[attr] = getattr(GLib.SpawnFlags, n)
    deprecated_attr("GLib", attr, "GLib.SpawnFlags." + n)
    __all__.append(attr)

for n in [
    "HIDDEN",
    "IN_MAIN",
    "REVERSE",
    "NO_ARG",
    "FILENAME",
    "OPTIONAL_ARG",
    "NOALIAS",
]:
    attr = "OPTION_FLAG_" + n
    globals()[attr] = getattr(GLib.OptionFlags, n)
    deprecated_attr("GLib", attr, "GLib.OptionFlags." + n)
    __all__.append(attr)

for n in ["UNKNOWN_OPTION", "BAD_VALUE", "FAILED"]:
    attr = "OPTION_ERROR_" + n
    deprecated_attr("GLib", attr, "GLib.OptionError." + n)
    globals()[attr] = getattr(GLib.OptionError, n)
    __all__.append(attr)


# these are not currently exported in GLib gir, presumably because they are
# platform dependent; so get them from our static bindings
for name in [
    "G_MINFLOAT",
    "G_MAXFLOAT",
    "G_MINDOUBLE",
    "G_MAXDOUBLE",
    "G_MINSHORT",
    "G_MAXSHORT",
    "G_MAXUSHORT",
    "G_MININT",
    "G_MAXINT",
    "G_MAXUINT",
    "G_MINLONG",
    "G_MAXLONG",
    "G_MAXULONG",
    "G_MAXSIZE",
    "G_MINSSIZE",
    "G_MAXSSIZE",
    "G_MINOFFSET",
    "G_MAXOFFSET",
]:
    attr = name.split("_", 1)[-1]
    globals()[attr] = getattr(_gi, name)
    __all__.append(attr)


class MainLoop(GLib.MainLoop):
    @staticmethod
    def __new__(cls: type[Self], context: MainContext | None = None) -> Self:
        return GLib.MainLoop.new(context, False)  # type: ignore[return-value]

    def run(self) -> None:
        with (
            register_sigint_fallback(self.quit),
            get_event_loop(self.get_context()).running(self.quit),
        ):
            super().run()


MainLoop = override(MainLoop)
__all__.append("MainLoop")


class MainContext(GLib.MainContext):
    # Backwards compatible API with default value
    def iteration(self, may_block: bool = True) -> bool:
        with get_event_loop(self).paused():
            return super().iteration(may_block)

    def query(self, max_priority: int) -> tuple[int, list[PollFD]]:
        """Determines information necessary to poll this main loop.

        :param max_priority: maximum priority source to check
        :returns:
          The timeout (msec) used for polling,
          and the list of poll fd's.

        Please also check the usage notes in the
        `C documention <https://docs.gtk.org/glib/method.MainContext.query.html>`__.
        """
        return main_context_query(self, max_priority)


MainContext = override(MainContext)
__all__.append("MainContext")


class Source(GLib.Source):
    # *args, **kwargs are kept for backwards compatibility.
    # See https://gitlab.gnome.org/GNOME/pygobject/-/merge_requests/506#note_2652436.
    @staticmethod
    def __new__(cls: type[Self], *args, **kwargs) -> Self:
        # use our custom pygi_source_new() here as g_source_new() is not
        # bindable
        source = source_new()
        source.__class__ = cls
        setattr(source, "__pygi_custom_source", True)
        return source

    def __del__(self) -> None:
        if hasattr(self, "__pygi_custom_source"):
            # We destroy and finalize the box from here, as GLib might hold
            # a reference (e.g. while the source is pending), delaying the
            # finalize call until a later point.
            self.destroy()
            self.finalize()
            self._clear_boxed()

    def finalize(self) -> None:
        pass

    def set_callback(
        self, func: typing.Callable[..., bool], *user_data: typing.Any
    ) -> None:
        if hasattr(self, "__pygi_custom_source"):
            # use our custom pygi_source_set_callback() if for a GSource object
            # with custom functions
            source_set_callback(self, func, *user_data)
        else:
            # otherwise, for Idle and Timeout, use the standard method
            super().set_callback(func, *user_data)

    def get_current_time(self) -> float:
        return GLib.get_real_time() * 0.000001

    get_current_time = deprecated(
        get_current_time, "GLib.Source.get_time() or GLib.get_real_time()"
    )

    # as get/set_priority are introspected, we can't use the static
    # property(get_priority, ..) here
    def __get_priority(self) -> int:
        return self.get_priority()

    def __set_priority(self, value: int) -> None:
        self.set_priority(value)

    priority = property(__get_priority, __set_priority)

    def __get_can_recurse(self) -> bool:
        return self.get_can_recurse()

    def __set_can_recurse(self, value: bool) -> None:
        self.set_can_recurse(value)

    can_recurse = property(__get_can_recurse, __set_can_recurse)


Source = override(Source)
__all__.append("Source")


class Idle(Source):
    @staticmethod
    def __new__(cls: type[Self], priority: int = GLib.PRIORITY_DEFAULT) -> Self:
        source = GLib.idle_source_new()
        source.__class__ = cls
        return source  # type: ignore[return-value]

    def __init__(self, priority: int = GLib.PRIORITY_DEFAULT) -> None:
        if priority != GLib.PRIORITY_DEFAULT:
            self.set_priority(priority)


__all__.append("Idle")


class Timeout(Source):
    @staticmethod
    def __new__(
        cls: type[Self], interval: int = 0, priority: int = GLib.PRIORITY_DEFAULT
    ) -> Self:
        source = GLib.timeout_source_new(interval)
        source.__class__ = cls
        return source  # type: ignore[return-value]

    def __init__(
        self, interval: int = 0, priority: int = GLib.PRIORITY_DEFAULT
    ) -> None:
        if priority != GLib.PRIORITY_DEFAULT:
            self.set_priority(priority)


__all__.append("Timeout")


# backwards compatible API
def idle_add(
    function: typing.Callable[..., bool],
    *user_data: typing.Any,
    priority: int = GLib.PRIORITY_DEFAULT_IDLE,
) -> int:
    return GLib.idle_add(priority, function, *user_data)


__all__.append("idle_add")


def timeout_add(
    interval: int,
    function: typing.Callable[..., bool],
    *user_data: typing.Any,
    priority: int = GLib.PRIORITY_DEFAULT,
) -> int:
    return GLib.timeout_add(priority, interval, function, *user_data)


__all__.append("timeout_add")


def timeout_add_seconds(
    interval: int,
    function: typing.Callable[..., bool],
    *user_data: typing.Any,
    priority: int = GLib.PRIORITY_DEFAULT,
) -> int:
    return GLib.timeout_add_seconds(priority, interval, function, *user_data)


__all__.append("timeout_add_seconds")


# The GI GLib API uses g_io_add_watch_full renamed to g_io_add_watch with
# a signature of (channel, priority, condition, func, user_data).
# Prior to PyGObject 3.8, this function was statically bound with an API closer to the
# non-full version with a signature of: (fd, condition, func, *user_data)
# We need to support this until we are okay with breaking API in a way which is
# not backwards compatible.
#
# This needs to take into account several historical APIs:
# - calling with an fd as first argument
# - calling with a Python file object as first argument (we keep this one as
#   it's really convenient and does not change the number of arguments)
# - calling without a priority as second argument
def _io_add_watch_get_args(channel, priority_, condition, *cb_and_user_data, **kwargs):
    if not isinstance(priority_, int) or isinstance(priority_, GLib.IOCondition):
        warnings.warn(
            "Calling io_add_watch without priority as second argument is deprecated",
            PyGIDeprecationWarning,
        )
        # shift the arguments around
        user_data = cb_and_user_data
        callback = condition
        condition = priority_
        if not callable(callback):
            raise TypeError("third argument must be callable")

        # backwards compatibility: Call with priority kwarg
        if "priority" in kwargs:
            warnings.warn(
                "Calling io_add_watch with priority keyword argument is deprecated, put it as second positional argument",
                PyGIDeprecationWarning,
            )
            priority_ = kwargs["priority"]
        else:
            priority_ = GLib.PRIORITY_DEFAULT
    else:
        if len(cb_and_user_data) < 1 or not callable(cb_and_user_data[0]):
            raise TypeError("expecting callback as fourth argument")
        callback = cb_and_user_data[0]
        user_data = cb_and_user_data[1:]

    # backwards compatibility: Allow calling with fd
    if isinstance(channel, int):

        def func_fdtransform(_, cond, *data):
            return callback(channel, cond, *data)

        real_channel = GLib.IOChannel.unix_new(channel)
    elif isinstance(channel, socket.socket) and sys.platform == "win32":

        def func_fdtransform(_, cond, *data):
            return callback(channel, cond, *data)

        real_channel = GLib.IOChannel.win32_new_socket(channel.fileno())
    elif hasattr(channel, "fileno"):
        # backwards compatibility: Allow calling with Python file
        def func_fdtransform(_, cond, *data):
            return callback(channel, cond, *data)

        real_channel = GLib.IOChannel.unix_new(channel.fileno())
    else:
        assert isinstance(channel, GLib.IOChannel)
        func_fdtransform = callback
        real_channel = channel

    return real_channel, priority_, condition, func_fdtransform, user_data


__all__.append("_io_add_watch_get_args")


def io_add_watch(*args, **kwargs) -> int:
    """io_add_watch(channel, priority, condition, func, *user_data) -> event_source_id."""
    channel, priority, condition, func, user_data = _io_add_watch_get_args(
        *args, **kwargs
    )
    return GLib.io_add_watch(channel, priority, condition, func, *user_data)


__all__.append("io_add_watch")


# backwards compatible API
class IOChannel(GLib.IOChannel):
    @staticmethod
    def __new__(
        cls: type[Self],
        filedes: int | None = None,
        filename: str | None = None,
        mode: str | None = None,
        hwnd: int | None = None,
    ) -> Self:
        if filedes is not None:
            return GLib.IOChannel.unix_new(filedes)  # type: ignore[return-value]
        if filename is not None:
            return GLib.IOChannel.new_file(filename, mode or "r")  # type: ignore[return-value]
        if hwnd is not None:
            return GLib.IOChannel.win32_new_fd(hwnd)  # type: ignore[return-value]
        raise TypeError(
            "either a valid file descriptor, file name, or window handle must be supplied"
        )

    def read(self, max_count: int = -1) -> bytes:
        """Reads data from a :obj:`~gi.repository.GLib.IOChannel`."""
        return io_channel_read(self, max_count)

    def read_chars(self, max_count: int = -1) -> bytes:
        """Alias for GLib.IOChannel.read()."""
        return self.read(max_count)

    def readline(self, size_hint: int = -1) -> str:
        # note, size_hint is just to maintain backwards compatible API; the
        # old static binding did not actually use it
        (_status, buf, _length, _terminator_pos) = self.read_line()
        if buf is None:
            return ""
        return buf

    def readlines(self, size_hint: int = -1) -> list[str]:
        # note, size_hint is just to maintain backwards compatible API;
        # the old static binding did not actually use it
        lines = []
        status = GLib.IOStatus.NORMAL
        while status == GLib.IOStatus.NORMAL:
            (status, buf, _length, _terminator_pos) = self.read_line()
            # note, this appends an empty line after EOF; this is
            # bug-compatible with the old static bindings
            if buf is None:
                buf = ""
            lines.append(buf)
        return lines

    def write(self, buf: str | bytes, buflen: int = -1) -> int:
        if not isinstance(buf, bytes):
            buf = buf.encode("UTF-8")
        if buflen == -1:
            buflen = len(buf)
        (_status, written) = self.write_chars(buf, buflen)
        return written

    def writelines(self, lines: typing.Iterable[str | bytes]) -> None:
        for line in lines:
            self.write(line)

    _whence_map = {0: GLib.SeekType.SET, 1: GLib.SeekType.CUR, 2: GLib.SeekType.END}

    def seek(self, offset: int, whence: int = 0) -> GLib.IOStatus:
        try:
            w = self._whence_map[whence]
        except KeyError:
            raise ValueError("invalid 'whence' value")
        return self.seek_position(offset, w)

    def add_watch(
        self,
        condition: IOCondition,
        callback: typing.Callable[..., bool],
        *user_data: typing.Any,
        priority: int = GLib.PRIORITY_DEFAULT,
    ) -> int:
        return io_add_watch(self, priority, condition, callback, *user_data)

    add_watch = deprecated(add_watch, "GLib.io_add_watch()")

    def __iter__(self):
        return self

    def __next__(self) -> str:
        (status, buf, _length, _terminator_pos) = self.read_line()
        if status == GLib.IOStatus.NORMAL:
            return buf
        raise StopIteration


IOChannel = override(IOChannel)
__all__.append("IOChannel")


class PollFD(GLib.PollFD):
    @staticmethod
    def __new__(cls: type[Self], fd: int, events: IOCondition) -> Self:
        pollfd = GLib.PollFD()
        pollfd.__class__ = cls
        return pollfd

    def __init__(self, fd: int, events: IOCondition) -> None:
        self.fd = fd
        self.events = events


PollFD = override(PollFD)
__all__.append("PollFD")


# The GI GLib API uses g_child_watch_add_full renamed to g_child_watch_add with
# a signature of (priority, pid, callback, data).
# Prior to PyGObject 3.8, this function was statically bound with an API closer to the
# non-full version with a signature of: (pid, callback, data=None, priority=GLib.PRIORITY_DEFAULT)
# We need to support this until we are okay with breaking API in a way which is
# not backwards compatible.
def _child_watch_add_get_args(priority_or_pid, pid_or_callback, *args, **kwargs):
    user_data = []

    if callable(pid_or_callback):
        warnings.warn(
            "Calling child_watch_add without priority as first argument is deprecated",
            PyGIDeprecationWarning,
        )
        pid = priority_or_pid
        callback = pid_or_callback
        if len(args) == 0:
            priority = kwargs.get("priority", GLib.PRIORITY_DEFAULT)
        elif len(args) == 1:
            user_data = args
            priority = kwargs.get("priority", GLib.PRIORITY_DEFAULT)
        elif len(args) == 2:
            user_data = [args[0]]
            priority = args[1]
        else:
            raise TypeError("expected at most 4 positional arguments")
    else:
        priority = priority_or_pid
        pid = pid_or_callback
        if "function" in kwargs:
            callback = kwargs["function"]
            user_data = args
        elif len(args) > 0 and callable(args[0]):
            callback = args[0]
            user_data = args[1:]
        else:
            raise TypeError("expected callback as third argument")

    if "data" in kwargs:
        if user_data:
            raise TypeError('got multiple values for "data" argument')
        user_data = (kwargs["data"],)

    return priority, pid, callback, user_data


# we need this to be accessible for unit testing
__all__.append("_child_watch_add_get_args")


def child_watch_add(*args, **kwargs):
    """child_watch_add(priority, pid, function, *data)."""
    priority, pid, function, data = _child_watch_add_get_args(*args, **kwargs)
    return GLib.child_watch_add(priority, pid, function, *data)


__all__.append("child_watch_add")


def get_current_time() -> float:
    return GLib.get_real_time() * 0.000001


get_current_time = deprecated(get_current_time, "GLib.get_real_time()")

__all__.append("get_current_time")


# backwards compatible API with default argument, and ignoring bytes_read
# output argument
def filename_from_utf8(utf8string: str, len: int = -1) -> str:
    return GLib.filename_from_utf8(utf8string, len)[0]


__all__.append("filename_from_utf8")


if hasattr(GLib, "unix_signal_add"):
    unix_signal_add_full = GLib.unix_signal_add
    __all__.append("unix_signal_add_full")
    deprecated_attr("GLib", "unix_signal_add_full", "GLib.unix_signal_add")


# obsolete constants for backwards compatibility
glib_version = (GLib.MAJOR_VERSION, GLib.MINOR_VERSION, GLib.MICRO_VERSION)
__all__.append("glib_version")
deprecated_attr(
    "GLib",
    "glib_version",
    "(GLib.MAJOR_VERSION, GLib.MINOR_VERSION, GLib.MICRO_VERSION)",
)

pyglib_version = version_info
__all__.append("pyglib_version")
deprecated_attr("GLib", "pyglib_version", "gi.version_info")

GLibPlatform = get_platform_specific_module(GLib)

if GLibPlatform:
    # Add support for using platform-specific GLib symbols.
    glib_globals = globals()

    for attr in dir(GLibPlatform):
        if attr.startswith("_"):
            continue

        original_attr, wrapper_attr = get_fallback_platform_specific_module_symbol(
            GLib, GLibPlatform, attr
        )

        if (
            wrapper_attr == attr
            and (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 88)
            and hasattr(GLib, wrapper_attr)
        ):
            warnings.warn(
                (
                    f"Name conflict for platform-specific symbol {GLibPlatform._namespace}.{attr}."
                    + " No wrapper will be created."
                ),
                PyGIWarning,
                stacklevel=2,
            )

        if (
            wrapper_attr in __all__
            or wrapper_attr in glib_globals
            or hasattr(GLib, wrapper_attr)
        ):
            continue

        glib_globals[wrapper_attr] = original_attr
        deprecated_attr(
            GLib._namespace, wrapper_attr, f"{GLibPlatform._namespace}.{attr}"
        )
        __all__.append(wrapper_attr)

if hasattr(GLibPlatform, "signal_add"):
    deprecated_attr(
        "GLib", "unix_signal_add_full", f"{GLibPlatform._namespace}.signal_add"
    )
