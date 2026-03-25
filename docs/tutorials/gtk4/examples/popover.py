import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class PopoverWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Popover Demo")

        self.set_default_size(300, 200)

        box = Gtk.Box(
            spacing=6,
            orientation=Gtk.Orientation.VERTICAL,
            halign=Gtk.Align.CENTER,
            valign=Gtk.Align.CENTER,
        )
        self.set_child(box)

        popover = Gtk.Popover(child=Gtk.Label(label="Item"))
        button = Gtk.MenuButton(label="Click Me", popover=popover)
        box.append(button)

        button2 = Gtk.Button(label="Click Me 2")
        button2.connect("clicked", self.on_button_clicked)
        box.append(button2)

        self.popover2 = Gtk.Popover(
            child=Gtk.Label(label="Another Popup!"), position=Gtk.PositionType.LEFT
        )
        self.popover2.set_parent(button2)

    def on_button_clicked(self, _button):
        self.popover2.popup()


def on_activate(app):
    win = PopoverWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
