import pytest

from gi.repository import GObject

try:
    from gi.repository import Gtk
except ImportError:
    Gtk = None


@pytest.mark.skipif(Gtk is None, reason="Gtk not available")
def test_subclass_of_subclass():
    class FirstLabel(Gtk.Label):
        __gtype_name__ = "FirstLabel"

    class SecondLabel(FirstLabel):
        __gtype_name__ = "SecondLabel"
        testprop = GObject.Property(type=int)

        def __init__(self):
            super().__init__()
            self.set_property("testprop", 34.5)

    xml = """\
    <?xml version="1.0" encoding="UTF-8"?>
    <interface domain="test">
      <requires lib="gtk" version="4.0" />
      <object class="SecondLabel" id="label">
        <property name="label">Hello, world</property>
      </object>
    </interface>
    """

    builder = Gtk.Builder.new_from_string(xml, len(xml))
    label = builder.get_object("label")

    assert label.props.testprop == 34
