import gc
import sys
import warnings
import weakref

import pytest

from gi import PyGIWarning
from gi.repository import GObject, Gio, Regress


class DerivedObj(Regress.TestObj):
    def __init__(self):
        super().__init__()


@pytest.mark.parametrize("obj_type", [Regress.TestObj, DerivedObj])
def test_same_object_if_object_is_alive(obj_type):
    container = Regress.TestObj()
    obj = obj_type()
    container.set_bare(obj)

    assert container.props.bare is obj


@pytest.mark.parametrize("obj_type", [Regress.TestObj, DerivedObj])
def test_object_without_instance_data_gets_deleted(obj_type):
    container = Regress.TestObj()
    obj = obj_type()
    container.props.bare = obj

    ref = weakref.ref(obj)
    del obj
    gc.collect()  # for pypy

    assert container.props.bare
    assert ref() is None


@pytest.mark.parametrize("obj_type", [Regress.TestObj, DerivedObj])
def test_object_with_instance_data_gets_deleted(obj_type):
    container = Regress.TestObj()
    obj = obj_type()
    obj.instance_data = 1
    container.props.bare = obj

    ref = weakref.ref(obj)
    del obj
    gc.collect()  # for pypy

    assert ref() is None


@pytest.mark.parametrize("obj_type", [Regress.TestObj, DerivedObj])
def test_object_with_instance_data_retains_data(obj_type):
    container = Regress.TestObj()
    obj = obj_type()
    obj.instance_data = 1
    container.props.bare = obj

    del obj
    new_obj = container.props.bare

    assert new_obj.instance_data == 1


class ValueObj(Regress.TestObj):
    py_int = GObject.Property(type=int)

    py_model = GObject.Property(type=Gio.ListModel)

    def __init__(self, value=0):
        super().__init__()
        self.value = value


def test_value_object_retains_init_value():
    container = Regress.TestObj()
    obj = ValueObj(42)
    container.props.bare = obj

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    assert new_obj.value == 42


def test_value_object_retains_property_value():
    container = Regress.TestObj()
    obj = ValueObj()
    obj.py_int = 42
    container.props.bare = obj

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    assert new_obj.py_int == 42


class MySpecialListStore(GObject.Object, Gio.ListModel):
    def __init__(self):
        super().__init__()

    def do_get_item_type(self):
        return Regress.TestObj.__gtype__

    def do_get_n_items(self):
        return 0

    def do_get_item(self, n):
        raise NotImplementedError


def test_value_object_retains_object_property_value():
    container = Regress.TestObj()
    obj = ValueObj()
    obj.model = MySpecialListStore()
    container.props.bare = obj

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    assert new_obj.model is not None


def test_object_with_property_binding():
    containers = [Regress.TestObj(), Regress.TestObj()]
    binder = Regress.TestObj()
    bindee = Regress.TestObj()

    binder.bind_property("bare", bindee, "bare", GObject.BindingFlags.SYNC_CREATE)
    containers[0].props.bare = binder
    containers[1].props.bare = bindee

    del binder
    del bindee

    gc.collect()  # for pypy

    new_binder = containers[0].props.bare
    new_bindee = containers[1].props.bare

    obj = Regress.TestObj()

    new_binder.props.bare = obj

    assert new_bindee.props.bare is obj


def test_object_with_signal_callback():
    container = Regress.TestObj()
    obj = ValueObj()

    def set_up_signal_handler(self):
        self.connect("first", lambda *_: self.set_property("int", 42))

    set_up_signal_handler(obj)

    container.props.bare = obj
    obj_id = id(obj)

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    new_obj.emit("first")

    assert obj_id == id(new_obj)
    assert new_obj.props.int == 42


with warnings.catch_warnings():
    warnings.filterwarnings("ignore", ".*use __slots__.*")

    class SlotObj(Regress.TestObj):
        __slots__ = ["py_int"]

        def __init__(self):
            super().__init__()


def test_class_with_slots_raises_warning():
    with warnings.catch_warnings(record=True) as warn:

        class _SlotClass(GObject.Object):
            __slots__ = ["py_int"]

    assert warn[0].category is PyGIWarning
    assert "SlotClass shouldn't use __slots__." in str(warn[0].message)


def test_subclass_with_slots_raises_warning():
    class BaseClass(GObject.Object):
        pass

    with warnings.catch_warnings(record=True) as warn:

        class _SlotClass(BaseClass):
            __slots__ = ["py_int"]

    assert warn[0].category is PyGIWarning
    assert "SlotClass shouldn't use __slots__." in str(warn[0].message)


def test_slot_object_can_be_created():
    container = Regress.TestObj()
    obj = SlotObj()
    obj.py_int = 42
    container.props.bare = obj

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    assert new_obj.py_int == 42


def test_slot_object_without_values_can_be_created():
    container = Regress.TestObj()
    obj = SlotObj()
    container.props.bare = obj

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    assert new_obj


def test_objects_with_cyclic_dependency_and_instance_dict():
    # Test with a cycle:
    #
    # a --> b --> c --> d
    #       ^-----------'
    a, b, c, d = [Regress.TestObj(int=i) for i in range(4)]

    b.name = "b"

    a.props.bare = b
    b.props.bare = c
    b.c = c
    c.props.bare = d
    c.d = d
    d.props.bare = b
    d.b = b

    del b, c, d

    gc.collect()
    gc.collect()

    new_b = a.props.bare
    new_c = new_b.props.bare
    new_d = new_c.props.bare

    assert new_d.props.bare is new_b
    assert a.props.bare.name == "b"


def test_objects_with_cyclic_dependency_without_instance_dict():
    # Test with a cycle:
    #
    # a --> b --> c --> d
    #       ^-----------'
    a, b, c, d = [Regress.TestObj(int=i) for i in range(4)]

    a.props.bare = b
    b.props.bare = c
    c.props.bare = d
    d.props.bare = b

    del b, c, d

    gc.collect()
    gc.collect()

    new_b = a.props.bare
    new_c = new_b.props.bare
    new_d = new_c.props.bare

    assert new_d.props.bare is new_b


def test_objects_with_cyclic_dependency_and_instance_dict_no_content():
    # Test with a cycle:
    #
    # b <-> a <-> c
    #       ^---> d
    a, b, c, d = [Regress.TestObj(int=i) for i in range(4)]

    b.name = "b"

    a.props.bare = b
    a.all = [b, c, d]
    b.props.bare = a
    b.a = a
    c.props.bare = a
    c.a = a
    d.props.bare = a
    d.a = a
    del b, c, d

    gc.collect()
    gc.collect()

    assert a.props.bare.name == "b"


def test_chained_objects_are_collected():
    # a --> b --> c
    a, b, c = [Regress.TestObj(int=i) for i in range(3)]
    a.b = b
    b.c = c

    aref = a.weak_ref()
    cref = c.weak_ref()
    del a, b, c

    gc.collect()
    gc.collect()  # some more for PyPy
    gc.collect()
    gc.collect()

    assert aref() is None
    assert cref() is None


def test_objects_can_be_deleted():
    a = Regress.TestObj()

    pyref = weakref.ref(a)
    gref = a.weak_ref()
    del a

    gc.collect()  # for PyPy
    gc.collect()

    assert pyref() is None
    assert gref() is None


@pytest.mark.skipif(
    sys.implementation.name == "pypy", reason="Doesn't play nice with PyPy GC"
)
def test_gobject_cycle_is_collected():
    # Test with a cycle:
    #
    # a --> b --> c
    # ^-----------'
    a, b, c = [Regress.TestObj(int=i) for i in range(3)]

    a.b = b
    b.c = c
    c.a = a

    ref = a.weak_ref()
    del a, b, c

    gc.collect()
    gc.collect()

    assert ref() is None


def test_object_with_post_init():
    class PostInit(Regress.TestObj):
        number = GObject.Property(
            type=int, flags=GObject.ParamFlags.READWRITE | GObject.ParamFlags.CONSTRUCT
        )

        def do_constructed(self):
            self.post_init_called = True
            assert self.number == 42

    obj = PostInit(number=42)

    assert obj.post_init_called
    assert obj.number == 42


def test_object_with_post_init_created_by_new():
    class PostInit(Regress.TestObj):
        number = GObject.Property(
            type=int, flags=GObject.ParamFlags.READWRITE | GObject.ParamFlags.CONSTRUCT
        )

        def do_constructed(self):
            self.post_init_called = True
            assert self.number == 42

    obj = GObject.new(PostInit, number=42)

    assert obj.post_init_called
    assert obj.number == 42


class InitializationOrder(Regress.TestObj):
    def __init__(self):
        self.order = ["__init__1"]
        super().__init__()
        self.order.append("__init__2")

    def do_constructed(self):
        self.order.append("do_constructed")


def test_object_constructed_after_init():
    obj = InitializationOrder()

    assert obj.order == ["__init__1", "do_constructed", "__init__2"]


def test_object_constructed_after_init_by_new():
    obj = GObject.new(InitializationOrder)

    assert obj.order == ["__init__1", "__init__2", "do_constructed"]


def test_object_with_post_init_and_interface():
    class PostInit(Regress.TestObj, Regress.TestInterface):
        number = GObject.Property(type=int)

        def __init__(self):
            self.post_init_called = 0
            super().__init__()

        def do_constructed(self):
            super().do_constructed()
            self.post_init_called += 1

    class SubPostInit(PostInit):
        def do_constructed(self):
            super().do_constructed()

    obj = PostInit()
    subobj = SubPostInit()

    assert obj.post_init_called == 1
    assert subobj.post_init_called == 1


def test_object_with_post_init_raises_exception():
    class PostInit(Regress.TestObj):
        def do_constructed(self):
            raise ValueError("Catch me")

    with pytest.raises(ValueError, match="Catch me"):
        PostInit()
    with pytest.raises(ValueError, match="Catch me"):
        GObject.new(PostInit)
