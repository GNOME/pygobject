import os
import platform
import random
import warnings

import pytest

from gi.repository import Gio, GLib, GObject
from gi import PyGIWarning


class Item(GObject.Object):
    _id = 0

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Item._id += 1
        self._id = self._id

    def __repr__(self):
        return str(self._id)


class NamedItem(Item):
    name = GObject.Property(type=str, default="")

    def __repr__(self):
        return self.props.name


def test_list_store_sort():
    store = Gio.ListStore()
    items = [NamedItem(name=n) for n in "cabx"]
    sorted_items = sorted(items, key=lambda i: i.props.name)

    user_data = [object(), object()]

    def sort_func(a, b, *args):
        assert list(args) == user_data
        assert isinstance(a, NamedItem)
        assert isinstance(b, NamedItem)

        def cmp(a, b):
            return (a > b) - (a < b)

        return cmp(a.props.name, b.props.name)

    store[:] = items
    assert store[:] != sorted_items
    store.sort(sort_func, *user_data)
    assert store[:] == sorted_items


def test_list_store_insert_sorted():
    store = Gio.ListStore()
    items = [NamedItem(name=n) for n in "cabx"]
    sorted_items = sorted(items, key=lambda i: i.props.name)

    user_data = [object(), object()]

    def sort_func(a, b, *args):
        assert list(args) == user_data
        assert isinstance(a, NamedItem)
        assert isinstance(b, NamedItem)

        def cmp(a, b):
            return (a > b) - (a < b)

        return cmp(a.props.name, b.props.name)

    for item in items:
        index = store.insert_sorted(item, sort_func, *user_data)
        assert isinstance(index, int)
    assert store[:] == sorted_items


def test_list_model_len():
    model = Gio.ListStore.new(Item)
    assert len(model) == 0
    assert not model
    for i in range(1, 10):
        model.append(Item())
        assert len(model) == i
    assert model
    model.remove_all()
    assert not model
    assert len(model) == 0


def test_list_model_get_item_simple():
    model = Gio.ListStore.new(Item)
    with pytest.raises(IndexError):
        model[0]
    first_item = Item()
    model.append(first_item)
    assert model[0] is first_item
    assert model[-1] is first_item
    second_item = Item()
    model.append(second_item)
    assert model[1] is second_item
    assert model[-1] is second_item
    assert model[-2] is first_item
    with pytest.raises(IndexError):
        model[-3]

    with pytest.raises(TypeError):
        model[object()]


def test_list_model_get_item_slice():
    model = Gio.ListStore.new(Item)
    source = [Item() for i in range(30)]
    for i in source:
        model.append(i)
    assert model[1:10] == source[1:10]
    assert model[1:-2] == source[1:-2]
    assert model[-4:-1] == source[-4:-1]
    assert model[-100:-1] == source[-100:-1]
    assert model[::-1] == source[::-1]
    assert model[:] == source[:]


def test_list_model_contains():
    model = Gio.ListStore.new(Item)
    item = Item()
    model.append(item)
    assert item in model
    assert Item() not in model
    with pytest.raises(TypeError):
        object() in model
    with pytest.raises(TypeError):
        None in model


def test_list_model_iter():
    model = Gio.ListStore.new(Item)
    item = Item()
    model.append(item)

    it = iter(model)
    assert next(it) is item
    repr(item)


def test_list_store_delitem_simple():
    store = Gio.ListStore.new(Item)
    store.append(Item())
    del store[0]
    assert not store
    with pytest.raises(IndexError):
        del store[0]
    with pytest.raises(IndexError):
        del store[-1]

    store.append(Item())
    with pytest.raises(IndexError):
        del store[-2]
    del store[-1]
    assert not store

    source = [Item(), Item()]
    store.append(source[0])
    store.append(source[1])
    del store[-1]
    assert store[:] == [source[0]]

    with pytest.raises(TypeError):
        del store[object()]


def test_list_store_delitem_slice():
    def do_del(count, key):
        events = []

        def on_changed(m, *args):
            events.append(args)

        store = Gio.ListStore.new(Item)
        source = [Item() for i in range(count)]
        for item in source:
            store.append(item)
        store.connect("items-changed", on_changed)
        source.__delitem__(key)
        store.__delitem__(key)
        assert source == store[:]
        return events

    values = [None, 1, -15, 3, -2, 0, -3, 5, 7]
    variants = set()
    for i in range(500):
        start = random.choice(values)
        stop = random.choice(values)
        step = random.choice(values)
        length = abs(random.choice(values) or 0)
        if step == 0:
            step += 1
        variants.add((length, start, stop, step))

    for length, start, stop, step in variants:
        do_del(length, slice(start, stop, step))

    # basics
    do_del(10, slice(None, None, None))
    do_del(10, slice(None, None, None))
    do_del(10, slice(None, None, -1))
    do_del(10, slice(0, 5, None))
    do_del(10, slice(0, 10, 1))
    do_del(10, slice(0, 10, 2))
    do_del(10, slice(14, 2, -1))

    # test some fast paths
    assert do_del(100, slice(None, None, None)) == [(0, 100, 0)]
    assert do_del(100, slice(None, None, -1)) == [(0, 100, 0)]
    assert do_del(100, slice(0, 50, 1)) == [(0, 50, 0)]


def test_list_store_setitem_simple():
    store = Gio.ListStore.new(Item)
    first = Item()
    store.append(first)

    class Wrong(GObject.Object):
        pass

    with pytest.raises(TypeError):
        store[0] = object()
    with pytest.raises(TypeError):
        store[0] = None
    with pytest.raises(TypeError):
        store[0] = Wrong()

    assert store[:] == [first]

    new = Item()
    store[0] = new
    assert len(store) == 1
    store[-1] = Item()
    assert len(store) == 1

    with pytest.raises(IndexError):
        store[1] = Item()
    with pytest.raises(IndexError):
        store[-2] = Item()

    store = Gio.ListStore.new(Item)
    source = [Item(), Item(), Item()]
    for item in source:
        store.append(item)
    new = Item()
    store[1] = new
    assert store[:] == [source[0], new, source[2]]

    with pytest.raises(TypeError):
        store[object()] = Item()


def test_list_store_setitem_slice():
    def do_set(count, key, new_count):
        if (
            count == 0
            and key.step is not None
            and platform.python_implementation() == "PyPy"
        ):
            # https://foss.heptapod.net/pypy/pypy/-/issues/2804
            return
        store = Gio.ListStore.new(Item)
        source = [Item() for i in range(count)]
        new = [Item() for i in range(new_count)]
        for item in source:
            store.append(item)
        source_error = None
        try:
            source.__setitem__(key, new)
        except ValueError as e:
            source_error = type(e)

        store_error = None
        try:
            store.__setitem__(key, new)
        except Exception as e:
            store_error = type(e)

        assert source_error == store_error
        assert source == store[:]

    values = [None, 1, -15, 3, -2, 0, 3, 4, 100]
    variants = set()
    for i in range(500):
        start = random.choice(values)
        stop = random.choice(values)
        step = random.choice(values)
        length = abs(random.choice(values) or 0)
        new = random.choice(values) or 0
        if step == 0:
            step += 1
        variants.add((length, start, stop, step, new))

    for length, start, stop, step, new in variants:
        do_set(length, slice(start, stop, step), new)

    # basics
    do_set(10, slice(None, None, None), 20)
    do_set(10, slice(None, None, None), 0)
    do_set(10, slice(None, None, -1), 20)
    do_set(10, slice(None, None, -1), 10)
    do_set(10, slice(0, 5, None), 20)
    do_set(10, slice(0, 10, 1), 0)

    # test iterators
    store = Gio.ListStore.new(Item)
    store[:] = iter([Item() for i in range(10)])
    assert len(store) == 10

    # make sure we do all or nothing
    store = Gio.ListStore.new(Item)
    with pytest.raises(TypeError):
        store[:] = [Item(), object()]
    assert len(store) == 0


def test_action_map_add_action_entries():
    actionmap = Gio.SimpleActionGroup()

    test_data = []

    def f(action, parameter, data):
        test_data.append("test back")

    actionmap.add_action_entries(
        (
            ("simple", f),
            ("with_type", f, "i"),
            ("with_state", f, "s", "'left'", f),
        )
    )
    assert actionmap.has_action("simple")
    assert actionmap.has_action("with_type")
    assert actionmap.has_action("with_state")
    actionmap.add_action_entries((("with_user_data", f),), "user_data")
    assert actionmap.has_action("with_user_data")

    with pytest.raises(TypeError):
        actionmap.add_action_entries((("invaild_type_string", f, "asdf"),))
    with pytest.raises(ValueError):
        actionmap.add_action_entries(
            (("stateless_with_change_state", f, None, None, f),)
        )

    actionmap.activate_action("simple")
    assert test_data[0] == "test back"


def test_types_init_warn():
    types = [
        Gio.DBusAnnotationInfo,
        Gio.DBusArgInfo,
        Gio.DBusMethodInfo,
        Gio.DBusSignalInfo,
        Gio.DBusInterfaceInfo,
        Gio.DBusNodeInfo,
    ]

    for t in types:
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            t()
            assert issubclass(warn[0].category, PyGIWarning)


def test_file_fspath():
    file, stream = Gio.File.new_tmp("TestGFile.XXXXXX")
    content = b"hello\0world\x7f!"
    stream.get_output_stream().write_bytes(GLib.Bytes(content))
    stream.close()
    path = file.peek_path()
    assert isinstance(path, str)
    fspath = file.__fspath__()
    assert isinstance(fspath, str)
    assert fspath == path

    with open(file, "rb") as file_like:
        assert file_like.read() == content


def test_file_fspath_with_no_path():
    file = Gio.File.new_for_path("")
    path = file.peek_path()
    # In older versions of GLib, creating a GFile for an empty path will result
    # in one representing the current pwd instead of a GDummyFile with no path.
    if path is None:
        with pytest.raises(TypeError):
            file.__fspath__()
    else:
        assert path == file.__fspath__()
        assert path == os.getcwd()


@pytest.mark.skipif(
    not hasattr(Gio.ListStore, "find_with_equal_func_full"),
    reason="ListStore.find_with_equal_func_full() is available in Gio 2.74",
)
def test_list_store_find_with_equal_func():
    def test(*user_data):
        class TestObject(GObject.Object):
            def __init__(self, val):
                super().__init__()
                self.val = val

        NUM = 5
        data = [TestObject(i) for i in range(NUM)]
        list_store = Gio.ListStore()
        for d in data:
            list_store.append(d)

        def equal_func(a, b, *data):
            assert data == user_data
            return a.val == b.val

        for i in range(NUM):
            res, position = list_store.find_with_equal_func(
                data[i], equal_func, *user_data
            )
            assert res
            assert position == i

        not_in_list_store = TestObject(NUM + 1)
        res, position = list_store.find_with_equal_func(
            not_in_list_store, equal_func, *user_data
        )
        assert not res

    test()
    test((100, "data"))
