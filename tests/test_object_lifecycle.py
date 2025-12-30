import gc
import weakref

import pytest

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
    # obj.model = Gio.ListStore.new(Regress.TestObj.__gtype__)
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
