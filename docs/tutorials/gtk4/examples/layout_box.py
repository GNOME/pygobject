import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class MyWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Hello World")

        box = Gtk.Box(spacing=6)
        self.set_child(box)

        button1 = Gtk.Button(label="Hello")
        button1.connect("clicked", self.on_button1_clicked)
        box.append(button1)

        button2 = Gtk.Button(label="Goodbye")
        button2.props.hexpand = True
        button2.connect("clicked", self.on_button2_clicked)
        box.append(button2)

    def on_button1_clicked(self, _widget):
        pass

    def on_button2_clicked(self, _widget):
        pass


def on_activate(app):
    # Create window
    win = MyWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
