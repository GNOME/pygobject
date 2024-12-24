import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class PopoverWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Popover Demo")

        self.set_default_size(300, 200)

        box = Gtk.Box(spacing=6, orientation=Gtk.Orientation.VERTICAL)
        box.props.halign = box.props.valign = Gtk.Align.CENTER
        self.set_child(box)

        popover = Gtk.Popover()
        popover_box = Gtk.Box()
        popover_box.append(Gtk.Label(label="Item"))
        popover.set_child(popover_box)

        button = Gtk.MenuButton(label="Click Me", popover=popover)
        box.append(button)

        button2 = Gtk.Button(label="Click Me 2")
        button2.connect("clicked", self.on_button_clicked)
        box.append(button2)

        self.popover2 = Gtk.Popover()
        self.popover2.set_child(Gtk.Label(label="Another Popup!"))
        self.popover2.set_parent(button2)
        self.popover2.props.position = Gtk.PositionType.LEFT

    def on_button_clicked(self, _button):
        self.popover2.popup()


def on_activate(app):
    win = PopoverWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
