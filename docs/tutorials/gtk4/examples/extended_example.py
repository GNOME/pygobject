import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class MyWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Hello World")

        self.button = Gtk.Button(label="Click Here")
        self.button.connect("clicked", self.on_button_clicked)
        self.set_child(self.button)

    def on_button_clicked(self, _widget):
        self.close()


def on_activate(app):
    # Create window
    win = MyWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
