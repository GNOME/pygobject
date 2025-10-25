import gc
import weakref

import pytest

from gi.repository import Regress


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
    def __init__(self, value):
        super().__init__()
        self._value = value


def test_value_object_retains_init_value():
    container = Regress.TestObj()
    obj = ValueObj(42)
    container.props.bare = obj

    del obj
    gc.collect()  # for pypy
    new_obj = container.props.bare

    assert new_obj._value == 42
