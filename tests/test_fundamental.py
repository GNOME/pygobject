import gc
import weakref

import pytest
from gi.repository import GObject, Regress

try:
    from gi.repository import Gtk

    GTK4 = Gtk._version == "4.0"
except ImportError:
    Gtk = None
    GTK4 = False


def test_constructor_no_data():
    obj = Regress.TestFundamentalSubObject()

    assert isinstance(obj, Regress.TestFundamentalSubObject)
    assert isinstance(obj, Regress.TestFundamentalObject)
    assert obj.refcount == 1
    assert obj.data is None


def test_constructor_with_data():
    with pytest.raises(TypeError):
        Regress.TestFundamentalSubObject(data="foo")


def test_create_fundamental_new_with_data():
    obj = Regress.TestFundamentalSubObject.new("foo")

    assert isinstance(obj, Regress.TestFundamentalSubObject)
    assert isinstance(obj, Regress.TestFundamentalObject)
    assert obj.refcount == 1
    assert obj.data == "foo"


@pytest.mark.skipif(
    not hasattr(Regress, "TestFundamentalObjectNoGetSetFunc"),
    reason="Old versions do not have this type",
)
def test_change_field():
    obj = Regress.TestFundamentalObjectNoGetSetFunc.new("foo")

    obj.data = "bar"

    assert obj.get_data() == "bar"


@pytest.mark.skipif(
    not hasattr(Regress, "TestFundamentalObjectNoGetSetFunc"),
    reason="Old versions do not have this type",
)
def test_call_method():
    obj = Regress.TestFundamentalObjectNoGetSetFunc.new("foo")

    assert obj.get_data() == "foo"


def test_create_fundamental_hidden_class_instance():
    obj = Regress.test_create_fundamental_hidden_class_instance()

    assert isinstance(obj, Regress.TestFundamentalObject)


def test_create_fundamental_refcount():
    obj = Regress.TestFundamentalSubObject.new("foo")

    assert obj.refcount == 1


def test_delete_fundamental_refcount():
    obj = Regress.TestFundamentalSubObject.new("foo")
    del obj

    gc.collect()


def test_value_set_fundamental_object():
    val = GObject.Value(Regress.TestFundamentalSubObject.__gtype__)
    obj = Regress.TestFundamentalSubObject()

    val.set_value(obj)

    assert val.get_value() == obj


def test_value_set_wrong_value():
    val = GObject.Value(Regress.TestFundamentalSubObject.__gtype__)

    with pytest.raises(TypeError, match="Fundamental type is required"):
        val.set_value(1)


def test_value_set_wrong_fundamental():
    val = GObject.Value(Regress.TestFundamentalSubObject.__gtype__)

    with pytest.raises(TypeError, match="Invalid fundamental type for assignment"):
        val.set_value(MyCustomFundamentalObject())


def test_array_of_fundamental_objects_in():
    assert Regress.test_array_of_fundamental_objects_in(
        [Regress.TestFundamentalSubObject()]
    )


def test_array_of_fundamental_objects_out():
    objs = Regress.test_array_of_fundamental_objects_out()

    assert len(objs) == 2
    assert all(isinstance(o, Regress.TestFundamentalObject) for o in objs)


def test_fundamental_argument_in():
    obj = Regress.TestFundamentalSubObject()

    assert Regress.test_fundamental_argument_in(obj)


def test_abstract_fundamental_type():
    with pytest.raises(TypeError):
        Regress.TestFundamentalObject()


def test_fundamental_argument_out():
    obj = Regress.TestFundamentalSubObject.new("data")
    other = Regress.test_fundamental_argument_out(obj)

    assert type(obj) is type(other)
    assert obj is not other
    assert obj.data == other.data


def test_multiple_objects():
    obj1 = Regress.TestFundamentalSubObject()
    obj2 = Regress.TestFundamentalSubObject()

    assert obj1 != obj2


def test_fundamental_weak_ref():
    obj = Regress.TestFundamentalSubObject()
    weak = weakref.ref(obj)

    assert weak() == obj

    del obj
    gc.collect()

    assert weak() is None


def test_fundamental_primitive_object():
    bitmask = Regress.Bitmask(2)

    assert bitmask.v == 2


def test_custom_fundamental_type_vfunc_override(capsys):
    obj = MyCustomFundamentalObject()
    del obj
    gc.collect()

    out = capsys.readouterr().out

    assert "MyCustomFundamentalObject.__init__" in out
    assert "MyCustomFundamentalObject.do_finalize" in out


@pytest.mark.skipif(not GTK4, reason="requires GTK 4")
def test_gtk_expression():
    obj = object()
    con = Gtk.ConstantExpression.new_for_value(obj)

    assert con.get_value() is obj


@pytest.mark.skipif(not GTK4, reason="requires GTK 4")
def test_gtk_string_filter_fundamental_property():
    expr = Gtk.ConstantExpression.new_for_value("one")
    filter = Gtk.StringFilter.new(expr)
    filter.props.expression = expr

    assert filter.get_expression() == expr
    assert filter.props.expression == expr


class MyCustomFundamentalObject(Regress.TestFundamentalObject):
    def __init__(self):
        print("MyCustomFundamentalObject.__init__")  # noqa T20
        super().__init__()

    def do_finalize(self):
        print("MyCustomFundamentalObject.do_finalize")  # noqa T20
