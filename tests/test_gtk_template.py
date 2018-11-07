# coding: UTF-8

from __future__ import absolute_import

import tempfile
import os
import pytest

Gtk = pytest.importorskip("gi.repository.Gtk")
GLib = pytest.importorskip("gi.repository.GLib")
GObject = pytest.importorskip("gi.repository.GObject")
Gio = pytest.importorskip("gi.repository.Gio")


from .helper import capture_exceptions


def new_gtype_name(_count=[0]):
    _count[0] += 1
    return "GtkTemplateTest%d" % _count[0]


def ensure_resource_registered():
    resource_path = "/org/gnome/pygobject/test/a.ui"

    def is_registered(path):
        try:
            Gio.resources_get_info(path, Gio.ResourceLookupFlags.NONE)
        except GLib.Error:
            return False
        return True

    if is_registered(resource_path):
        return resource_path

    gresource_data = (
        b'GVariant\x00\x00\x00\x00\x00\x00\x00\x00\x18\x00\x00\x00'
        b'\xc8\x00\x00\x00\x00\x00\x00(\x06\x00\x00\x00\x00\x00\x00\x00'
        b'\x00\x00\x00\x00\x01\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00'
        b'\x06\x00\x00\x00KP\x90\x0b\x03\x00\x00\x00\xc8\x00\x00\x00'
        b'\x04\x00L\x00\xcc\x00\x00\x00\xd0\x00\x00\x00\xb0\xb7$0'
        b'\x00\x00\x00\x00\xd0\x00\x00\x00\x06\x00L\x00\xd8\x00\x00\x00'
        b'\xdc\x00\x00\x00f\xc30\xd1\x01\x00\x00\x00\xdc\x00\x00\x00'
        b'\n\x00L\x00\xe8\x00\x00\x00\xec\x00\x00\x00\xd4\xb5\x02\x00'
        b'\xff\xff\xff\xff\xec\x00\x00\x00\x01\x00L\x00\xf0\x00\x00\x00'
        b'\xf4\x00\x00\x005H}\xe3\x02\x00\x00\x00\xf4\x00\x00\x00'
        b'\x05\x00L\x00\xfc\x00\x00\x00\x00\x01\x00\x00\xa2^\xd6t'
        b'\x04\x00\x00\x00\x00\x01\x00\x00\x04\x00v\x00\x08\x01\x00\x00'
        b'\xa5\x01\x00\x00org/\x01\x00\x00\x00gnome/\x00\x00\x02\x00\x00\x00'
        b'pygobject/\x00\x00\x04\x00\x00\x00/\x00\x00\x00\x00\x00\x00\x00'
        b'test/\x00\x00\x00\x05\x00\x00\x00a.ui\x00\x00\x00\x00'
        b'\x8d\x00\x00\x00\x00\x00\x00\x00<interface>\n  <template class="G'
        b'tkTemplateTestResource" parent="GtkBox">\n  <property name="spaci'
        b'ng">42</property>\n  </template>\n</interface>\n\x00\x00(uuay)'
    )

    resource = Gio.Resource.new_from_data(GLib.Bytes.new(gresource_data))
    Gio.resources_register(resource)
    assert is_registered(resource_path)
    return resource_path


def test_allow_init_template_call():

    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
  </template>
</interface>
""".format(type_name)

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        def __init__(self):
            super(Foo, self).__init__()
            self.init_template()

    # Stop current pygobject from handling the initialisation
    del Foo.__dontuse_ginstance_init__

    Foo()


def test_init_template_second_instance():
    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <child>
      <object class="GtkLabel" id="label">
      </object>
    </child>
  </template>
</interface>
""".format(type_name)

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        label = Gtk.Template.Child("label")

        def __init__(self):
            super(Foo, self).__init__()
            self.init_template()

    # Stop current pygobject from handling the initialisation
    del Foo.__dontuse_ginstance_init__

    foo = Foo()
    assert isinstance(foo.label, Gtk.Label)

    foo2 = Foo()
    assert isinstance(foo2.label, Gtk.Label)


def test_main_example():

    type_name = new_gtype_name()

    example_xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <property name="orientation">GTK_ORIENTATION_HORIZONTAL</property>
    <property name="spacing">4</property>
    <child>
      <object class="GtkButton" id="hello_button">
        <property name="label">Hello World</property>
        <signal name="clicked" handler="hello_button_clicked"
                object="{0}" swapped="no"/>
        <signal name="clicked" handler="hello_button_clicked_after"
                object="{0}" swapped="no" after="yes"/>
      </object>
    </child>
    <child>
      <object class="GtkButton" id="goodbye_button">
        <property name="label">Goodbye World</property>
        <signal name="clicked" handler="goodbye_button_clicked"/>
        <signal name="clicked" handler="goodbye_button_clicked_after"
                after="yes"/>
      </object>
    </child>
  </template>
</interface>
""".format(type_name)

    @Gtk.Template.from_string(example_xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        def __init__(self):
            super(Foo, self).__init__()
            self.callback_hello = []
            self.callback_hello_after = []
            self.callback_goodbye = []
            self.callback_goodbye_after = []

        @Gtk.Template.Callback("hello_button_clicked")
        def _hello_button_clicked(self, *args):
            self.callback_hello.append(args)

        @Gtk.Template.Callback("hello_button_clicked_after")
        def _hello_after(self, *args):
            self.callback_hello_after.append(args)

        _hello_button = Gtk.Template.Child("hello_button")

        goodbye_button = Gtk.Template.Child()

        @Gtk.Template.Callback("goodbye_button_clicked")
        def _goodbye_button_clicked(self, *args):
            self.callback_goodbye.append(args)

        @Gtk.Template.Callback("goodbye_button_clicked_after")
        def _goodbye_after(self, *args):
            self.callback_goodbye_after.append(args)

    w = Foo()
    assert w.__gtype__.name == type_name
    assert w.props.orientation == Gtk.Orientation.HORIZONTAL
    assert w.props.spacing == 4
    assert isinstance(w._hello_button, Gtk.Button)
    assert w._hello_button.props.label == "Hello World"
    assert isinstance(w.goodbye_button, Gtk.Button)
    assert w.goodbye_button.props.label == "Goodbye World"

    assert w.callback_hello == []
    w._hello_button.clicked()
    assert w.callback_hello == [(w,)]
    assert w.callback_hello_after == [(w,)]

    assert w.callback_goodbye == []
    w.goodbye_button.clicked()
    assert w.callback_goodbye == [(w.goodbye_button,)]
    assert w.callback_goodbye_after == [(w.goodbye_button,)]


def test_duplicate_handler():

    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <child>
      <object class="GtkButton" id="hello_button">
        <signal name="clicked" handler="hello_button_clicked">
      </object>
    </child>
  </template>
</interface>
""".format(type_name)

    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        @Gtk.Template.Callback("hello_button_clicked")
        def _hello_button_clicked(self, *args):
            pass

        @Gtk.Template.Callback()
        def hello_button_clicked(self, *args):
            pass

    with pytest.raises(RuntimeError, match=".*hello_button_clicked.*"):
        Gtk.Template.from_string(xml)(Foo)


def test_duplicate_child():
    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <child>
      <object class="GtkButton" id="hello_button" />
    </child>
  </template>
</interface>
""".format(type_name)

    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        foo = Gtk.Template.Child("hello_button")
        hello_button = Gtk.Template.Child()

    with pytest.raises(RuntimeError, match=".*hello_button.*"):
        Gtk.Template.from_string(xml)(Foo)


def test_nonexist_handler():
    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
  </template>
</interface>
""".format(type_name)

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        @Gtk.Template.Callback("nonexit")
        def foo(self, *args):
            pass

    with capture_exceptions() as exc_info:
        Foo()
    assert "nonexit" in str(exc_info[0].value)
    assert exc_info[0].type is RuntimeError


def test_missing_handler_callback():
    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <child>
      <object class="GtkButton" id="hello_button">
        <signal name="clicked" handler="i_am_not_used_in_python" />
      </object>
    </child>
  </template>
</interface>
""".format(type_name)

    class Foo(Gtk.Box):
        __gtype_name__ = type_name

    Gtk.Template.from_string(xml)(Foo)()


def test_handler_swapped_not_supported():

    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <child>
      <object class="GtkButton" id="hello_button">
        <signal name="clicked" handler="hello_button_clicked"
                object="{0}" swapped="yes" />
      </object>
    </child>
  </template>
</interface>
""".format(type_name)

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        hello_button = Gtk.Template.Child()

        @Gtk.Template.Callback("hello_button_clicked")
        def foo(self, *args):
            pass

    with capture_exceptions() as exc_info:
        Foo()
    assert "G_CONNECT_SWAPPED" in str(exc_info[0].value)


def test_handler_class_staticmethod():

    type_name = new_gtype_name()

    xml = """\
<interface>
  <template class="{0}" parent="GtkBox">
    <child>
      <object class="GtkButton" id="hello_button">
        <signal name="clicked" handler="clicked_class" />
        <signal name="clicked" handler="clicked_static" />
      </object>
    </child>
  </template>
</interface>
""".format(type_name)

    signal_args_class = []
    signal_args_static = []

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

        hello_button = Gtk.Template.Child()

        @Gtk.Template.Callback("clicked_class")
        @classmethod
        def cb1(*args):
            signal_args_class.append(args)

        @Gtk.Template.Callback("clicked_static")
        @staticmethod
        def cb2(*args):
            signal_args_static.append(args)

    foo = Foo()
    foo.hello_button.clicked()
    assert signal_args_class == [(Foo, foo.hello_button)]
    assert signal_args_static == [(foo.hello_button,)]


def test_check_decorated_class():

    NonWidget = type("Foo", (object,), {})
    with pytest.raises(TypeError, match=".*on Widgets.*"):
        Gtk.Template.from_string("")(NonWidget)

    Widget = type("Foo", (Gtk.Widget,), {"__gtype_name__": new_gtype_name()})
    with pytest.raises(TypeError, match=".*Cannot nest.*"):
        Gtk.Template.from_string("")(Gtk.Template.from_string("")(Widget))

    Widget = type("Foo", (Gtk.Widget,), {})
    with pytest.raises(TypeError, match=".*__gtype_name__.*"):
        Gtk.Template.from_string("")(Widget)

    with pytest.raises(TypeError, match=".*on Widgets.*"):
        Gtk.Template.from_string("")(object())


@pytest.mark.skipif(Gtk._version == "4.0", reason="errors out first with gtk4")
def test_subclass_fail():
    @Gtk.Template.from_string("")
    class Base(Gtk.Widget):
        __gtype_name__ = new_gtype_name()

    with capture_exceptions() as exc_info:
        type("Sub", (Base,), {})()
    assert "not allowed at this time" in str(exc_info[0].value)
    assert exc_info[0].type is TypeError


def test_from_file():
    fd, name = tempfile.mkstemp()
    try:
        os.close(fd)

        type_name = new_gtype_name()

        with open(name, "wb") as h:
            h.write(u"""\
    <interface>
      <template class="{0}" parent="GtkBox">
      <property name="spacing">42</property>
      </template>
    </interface>
    """.format(type_name).encode())

        @Gtk.Template.from_file(name)
        class Foo(Gtk.Box):
            __gtype_name__ = type_name

        foo = Foo()
        assert foo.props.spacing == 42
    finally:
        os.remove(name)


def test_property_override():
    type_name = new_gtype_name()

    xml = """\
    <interface>
      <template class="{0}" parent="GtkBox">
      <property name="spacing">42</property>
      </template>
    </interface>
""".format(type_name)

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

    foo = Foo()
    assert foo.props.spacing == 42

    foo = Foo(spacing=124)
    assert foo.props.spacing == 124


def test_from_file_non_exist():
    dirname = tempfile.mkdtemp()
    try:
        path = os.path.join(dirname, "noexist")

        Widget = type(
            "Foo", (Gtk.Widget,), {"__gtype_name__": new_gtype_name()})
        with pytest.raises(GLib.Error, match=".*No such file.*"):
            Gtk.Template.from_file(path)(Widget)
    finally:
        os.rmdir(dirname)


def test_from_string_bytes():
    type_name = new_gtype_name()

    xml = u"""\
    <interface>
      <template class="{0}" parent="GtkBox">
      <property name="spacing">42</property>
      </template>
    </interface>
    """.format(type_name).encode()

    @Gtk.Template.from_string(xml)
    class Foo(Gtk.Box):
        __gtype_name__ = type_name

    foo = Foo()
    assert foo.props.spacing == 42


def test_from_resource():
    resource_path = ensure_resource_registered()

    @Gtk.Template.from_resource(resource_path)
    class Foo(Gtk.Box):
        __gtype_name__ = "GtkTemplateTestResource"

    foo = Foo()
    assert foo.props.spacing == 42


def test_from_resource_non_exit():
    Widget = type("Foo", (Gtk.Widget,), {"__gtype_name__": new_gtype_name()})
    with pytest.raises(GLib.Error, match=".*/or/gnome/pygobject/noexit.*"):
        Gtk.Template.from_resource("/or/gnome/pygobject/noexit")(Widget)


def test_constructors():
    with pytest.raises(TypeError):
        Gtk.Template()

    with pytest.raises(TypeError):
        Gtk.Template(foo=1)

    Gtk.Template(filename="foo")
    Gtk.Template(resource_path="foo")
    Gtk.Template(string="foo")

    with pytest.raises(TypeError):
        Gtk.Template(filename="foo", resource_path="bar")

    with pytest.raises(TypeError):
        Gtk.Template(filename="foo", nope="bar")

    Gtk.Template.from_string("bla")
    Gtk.Template.from_resource("foo")
    Gtk.Template.from_file("foo")


def test_child_construct():
    Gtk.Template.Child()
    Gtk.Template.Child("name")
    with pytest.raises(TypeError):
        Gtk.Template.Child("name", True)
    Gtk.Template.Child("name", internal=True)
    with pytest.raises(TypeError):
        Gtk.Template.Child("name", internal=True, something=False)


def test_internal_child():

    main_type_name = new_gtype_name()

    xml = """\
    <interface>
      <template class="{0}" parent="GtkBox">
        <child>
          <object class="GtkBox" id="somechild">
            <property name="margin">42</property>
          </object>
        </child>
      </template>
    </interface>
    """.format(main_type_name)

    @Gtk.Template.from_string(xml)
    class MainThing(Gtk.Box):
        __gtype_name__ = main_type_name

        somechild = Gtk.Template.Child(internal=True)

    thing = MainThing()
    assert thing.somechild.props.margin == 42

    other_type_name = new_gtype_name()

    xml = """\
    <interface>
      <template class="{0}" parent="GtkBox">
        <child>
          <object class="{1}">
            <child internal-child="somechild">
              <object class="GtkBox">
                <property name="margin">24</property>
                <child>
                  <object class="GtkLabel">
                    <property name="label">foo</property>
                  </object>
                </child>
              </object>
            </child>
          </object>
        </child>
      </template>
    </interface>
    """.format(other_type_name, main_type_name)

    @Gtk.Template.from_string(xml)
    class OtherThing(Gtk.Box):
        __gtype_name__ = other_type_name

    other = OtherThing()
    child = other.get_children()[0]
    assert isinstance(child, MainThing)
    child = child.get_children()[0]
    assert isinstance(child, Gtk.Box)
    assert child.props.margin == 24
    child = child.get_children()[0]
    assert isinstance(child, Gtk.Label)
    assert child.props.label == "foo"
