import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class CenterBoxWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, default_width=400, title="CenterBox Example")

        box = Gtk.CenterBox()
        self.set_child(box)

        button1 = Gtk.Button(label="Start")
        box.set_start_widget(button1)

        label = Gtk.Label(label="Center")
        box.set_center_widget(label)

        button2 = Gtk.Button(label="End")
        box.set_end_widget(button2)


def on_activate(app):
    # Create window
    win = CenterBoxWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
