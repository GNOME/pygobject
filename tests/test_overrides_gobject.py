import pytest

from gi import PyGIDeprecationWarning
from gi.repository import GObject, GLib, GIMarshallingTests

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

    with pytest.raises(TypeError, match=r"GObject.Value needs to be initialized first"):
        v.set_value(None)

    assert v.get_value() is None


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

    with pytest.raises(TypeError):
        v.set_value(object())

    with pytest.raises(TypeError):
        v.set_value(None)


def test_value_float():
    v = GObject.Value(GObject.TYPE_FLOAT)

    for getter, setter in [(v.get_value, v.set_value), (v.get_float, v.set_float)]:
        assert getter() == 0.0
        setter(0)
        assert getter() == 0

        setter(GLib.MAXFLOAT)
        assert getter() == GLib.MAXFLOAT

        setter(GLib.MINFLOAT)
        assert getter() == GLib.MINFLOAT

        setter(-GLib.MAXFLOAT)
        assert getter() == -GLib.MAXFLOAT

        with pytest.raises(OverflowError):
            setter(GLib.MAXFLOAT * 2)

        with pytest.raises(OverflowError):
            setter(-GLib.MAXFLOAT * 2)

        with pytest.raises(TypeError):
            setter(object())

        with pytest.raises(TypeError):
            setter(None)

        with pytest.raises(TypeError):
            setter(1j)

        v.reset()


def test_value_double():
    v = GObject.Value(GObject.TYPE_DOUBLE)
    assert v.get_value() == 0.0
    v.set_value(0)
    assert v.get_value() == 0

    v.set_value(GLib.MAXDOUBLE)
    assert v.get_value() == GLib.MAXDOUBLE

    v.set_value(GLib.MINDOUBLE)
    assert v.get_value() == GLib.MINDOUBLE

    v.set_value(-GLib.MAXDOUBLE)
    assert v.get_value() == -GLib.MAXDOUBLE

    with pytest.raises(TypeError):
        v.set_value(object())

    with pytest.raises(TypeError):
        v.set_value(None)

    with pytest.raises(TypeError):
        v.set_value(1j)


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

    with pytest.raises(TypeError):
        v.set_value(object())

    with pytest.raises(TypeError):
        v.set_value(None)


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
    variant = GLib.Variant("i", 42)
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
    for getter, setter in [(v.get_value, v.set_value), (v.get_string, v.set_string)]:
        assert getter() is None

        with pytest.raises(TypeError):
            setter(b"bar")

        setter("quux")
        assert getter() == "quux"
        assert isinstance(getter(), str)

        setter(None)
        assert getter() is None

        v.reset()


def test_value_pyobject():
    class Foo:
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
        v.set_value("a")

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
        v.set_value("a")

    with pytest.raises(OverflowError):
        v.set_value(256)


def test_value_set_boxed_deprecate_non_boxed():
    v = GObject.Value(GObject.TYPE_POINTER)
    with pytest.warns(PyGIDeprecationWarning):
        v.get_boxed()
    with pytest.warns(PyGIDeprecationWarning):
        v.set_boxed(None)


def test_value_boolean():
    v = GObject.Value(GObject.TYPE_BOOLEAN)
    for getter, setter in [(v.get_value, v.set_value), (v.get_boolean, v.set_boolean)]:
        assert getter() is False
        assert isinstance(getter(), bool)

        setter(42)
        assert getter() is True
        setter(-1)
        assert getter() is True
        setter(0)
        assert getter() is False

        setter([])
        assert getter() is False
        setter(["foo"])
        assert getter() is True

        setter(None)
        assert getter() is False
        v.reset()


def test_value_enum():
    t = GIMarshallingTests.GEnum
    v = GObject.Value(t)

    for getter, setter in [(v.get_value, v.set_value), (v.get_enum, v.set_enum)]:
        assert v.g_type == t.__gtype__
        assert getter() == 0

        setter(t.VALUE1)
        assert getter() == t.VALUE1
        # FIXME: we should try to return an enum type
        assert type(getter()) is int

        setter(2424242)
        assert getter() == 2424242

        setter(-1)
        assert getter() == -1

        with pytest.raises(TypeError):
            setter(object())

        with pytest.raises(TypeError):
            setter(None)

        v.reset()


def test_value_object():
    v = GObject.Value(GIMarshallingTests.Object)
    assert v.g_type.is_a(GObject.TYPE_OBJECT)

    for getter, setter in [(v.get_value, v.set_value), (v.get_object, v.set_object)]:
        assert getter() is None

        setter(None)
        assert getter() is None

        obj = GIMarshallingTests.Object()
        setter(obj)
        assert getter() is obj

        with pytest.raises(TypeError):
            setter(object())

        v.reset()
