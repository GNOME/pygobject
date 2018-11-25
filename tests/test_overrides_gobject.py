# -*- coding: utf-8 -*-

from __future__ import absolute_import

import pytest

from gi import PyGIDeprecationWarning
from gi.repository import GObject, GLib

from gi._compat import PY2
from .helper import ignore_gi_deprecation_warnings


def test_stop_emission_deprec():
    class TestObject(GObject.GObject):
        int_prop = GObject.Property(default=0, type=int)

    obj = TestObject()

    def notify_callback(obj, *args):
        with pytest.warns(PyGIDeprecationWarning):
            obj.stop_emission("notify::int-prop")

        with pytest.warns(PyGIDeprecationWarning):
            obj.emit_stop_by_name("notify::int-prop")

        obj.stop_emission_by_name("notify::int-prop")

    obj.connect("notify::int-prop", notify_callback)
    obj.notify("int-prop")


def test_signal_parse_name():
    obj = GObject.GObject()
    assert GObject.signal_parse_name("notify", obj, True) == (1, 0)

    with pytest.raises(ValueError):
        GObject.signal_parse_name("foobar", obj, True)


def test_signal_query():
    obj = GObject.GObject()
    res = GObject.signal_query("notify", obj)
    assert res.signal_name == "notify"
    assert res.itype == obj.__gtype__

    res = GObject.signal_query("foobar", obj)
    assert res is None


def test_value_repr():
    v = GObject.Value()
    assert repr(v) == "<Value (invalid) None>"

    v = GObject.Value(int, 0)
    assert repr(v) == "<Value (gint) 0>"


def test_value_no_init():
    v = GObject.Value()
    with pytest.raises(TypeError):
        v.set_value(0)
    v.init(GObject.TYPE_LONG)
    assert v.get_value() == 0
    v.set_value(0)


def test_value_invalid_type():
    v = GObject.Value()
    assert v.g_type == GObject.TYPE_INVALID
    assert isinstance(GObject.TYPE_INVALID, GObject.GType)
    with pytest.raises(ValueError, match="Invalid GType"):
        v.init(GObject.TYPE_INVALID)


def test_value_long():
    v = GObject.Value(GObject.TYPE_LONG)
    assert v.get_value() == 0
    v.set_value(0)
    assert v.get_value() == 0

    v.set_value(GLib.MAXLONG)
    assert v.get_value() == GLib.MAXLONG

    v.set_value(GLib.MINLONG)
    assert v.get_value() == GLib.MINLONG

    with pytest.raises(OverflowError):
        v.set_value(GLib.MAXLONG + 1)

    with pytest.raises(OverflowError):
        v.set_value(GLib.MINLONG - 1)


def test_value_ulong():
    v = GObject.Value(GObject.TYPE_ULONG)
    assert v.get_value() == 0
    v.set_value(0)
    assert v.get_value() == 0

    v.set_value(GLib.MAXULONG)
    assert v.get_value() == GLib.MAXULONG

    with pytest.raises(OverflowError):
        v.set_value(GLib.MAXULONG + 1)

    with pytest.raises(OverflowError):
        v.set_value(-1)


def test_value_uint64():
    v = GObject.Value(GObject.TYPE_UINT64)
    assert v.get_value() == 0
    v.set_value(0)
    assert v.get_value() == 0

    v.set_value(GLib.MAXUINT64)
    assert v.get_value() == GLib.MAXUINT64

    with pytest.raises(OverflowError):
        v.set_value(GLib.MAXUINT64 + 1)

    with pytest.raises(OverflowError):
        v.set_value(-1)


def test_value_int64():
    v = GObject.Value(GObject.TYPE_INT64)
    assert v.get_value() == 0
    v.set_value(0)
    assert v.get_value() == 0

    v.set_value(GLib.MAXINT64)
    assert v.get_value() == GLib.MAXINT64
    v.set_value(GLib.MININT64)
    assert v.get_value() == GLib.MININT64

    with pytest.raises(OverflowError):
        v.set_value(GLib.MAXINT64 + 1)

    with pytest.raises(OverflowError):
        v.set_value(GLib.MININT64 - 1)


def test_value_pointer():
    v = GObject.Value(GObject.TYPE_POINTER)
    assert v.get_value() == 0
    v.set_value(42)
    assert v.get_value() == 42
    v.set_value(0)
    assert v.get_value() == 0


def test_value_unichar():
    assert GObject.TYPE_UNICHAR == GObject.TYPE_UINT

    v = GObject.Value(GObject.TYPE_UNICHAR)
    assert v.get_value() == 0
    v.set_value(42)
    assert v.get_value() == 42

    v.set_value(GLib.MAXUINT)
    assert v.get_value() == GLib.MAXUINT


def test_value_gtype():
    class TestObject(GObject.GObject):
        pass

    v = GObject.Value(GObject.TYPE_GTYPE)
    assert v.get_value() == GObject.TYPE_INVALID
    v.set_value(TestObject.__gtype__)
    assert v.get_value() == TestObject.__gtype__
    v.set_value(TestObject)
    assert v.get_value() == TestObject.__gtype__

    with pytest.raises(TypeError):
        v.set_value(None)


def test_value_variant():
    v = GObject.Value(GObject.TYPE_VARIANT)
    assert v.get_value() is None
    variant = GLib.Variant('i', 42)
    v.set_value(variant)
    assert v.get_value() == variant

    v.set_value(None)
    assert v.get_value() is None

    with pytest.raises(TypeError):
        v.set_value(object())


def test_value_param():
    # FIXME: set_value and get_value trigger a critical
    # GObject.Value(GObject.TYPE_PARAM)
    pass


def test_value_string():
    v = GObject.Value(GObject.TYPE_STRING)
    assert v.get_value() is None

    if PY2:
        v.set_value(b"bar")
        assert v.get_value() == b"bar"

        v.set_value(u"öäü")
        assert v.get_value().decode("utf-8") == u"öäü"
    else:
        with pytest.raises(TypeError):
            v.set_value(b"bar")

    v.set_value(u"quux")
    assert v.get_value() == u"quux"
    assert isinstance(v.get_value(), str)

    with pytest.raises(TypeError):
        v.set_value(None)


def test_value_pyobject():
    class Foo(object):
        pass

    v = GObject.Value(GObject.TYPE_PYOBJECT)
    assert v.get_value() is None
    for obj in [Foo(), None, 42, "foo"]:
        v.set_value(obj)
        assert v.get_value() == obj


@ignore_gi_deprecation_warnings
def test_value_char():
    v = GObject.Value(GObject.TYPE_CHAR)
    assert v.get_value() == 0
    v.set_value(42)
    assert v.get_value() == 42
    v.set_value(-1)
    assert v.get_value() == -1
    v.set_value(b"a")
    assert v.get_value() == 97
    v.set_value(b"\x00")
    assert v.get_value() == 0

    with pytest.raises(TypeError):
        v.set_value(u"a")

    with pytest.raises(OverflowError):
        v.set_value(128)


def test_value_uchar():
    v = GObject.Value(GObject.TYPE_UCHAR)
    assert v.get_value() == 0
    v.set_value(200)
    assert v.get_value() == 200
    v.set_value(b"a")
    assert v.get_value() == 97
    v.set_value(b"\x00")
    assert v.get_value() == 0

    with pytest.raises(TypeError):
        v.set_value(u"a")

    with pytest.raises(OverflowError):
        v.set_value(256)
