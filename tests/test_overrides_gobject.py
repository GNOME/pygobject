from __future__ import absolute_import

import pytest

from gi import PyGIDeprecationWarning
from gi.repository import GObject, GLib

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
    # FIXME: this complains that the value isn't a GType
    # GObject.Value(GObject.TYPE_INVALID)
    pass


def test_value_long():
    v = GObject.Value(GObject.TYPE_LONG)
    assert v.get_value() == 0
    v.set_value(0)
    assert v.get_value() == 0

    v.set_value(GLib.MAXLONG)
    assert v.get_value() == GLib.MAXLONG

    v.set_value(GLib.MINLONG)
    assert v.get_value() == GLib.MINLONG


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

    # FIXME: triggers an assert
    # assert v.get_value() == variant

    v.set_value(None)
    assert v.get_value() is None


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
