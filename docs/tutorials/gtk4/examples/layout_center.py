import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class CenterBoxWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, default_width=400, title="CenterBox Example")

        box = Gtk.CenterBox(
            start_widget=Gtk.Button(label="Start"),
            center_widget=Gtk.Label(label="Center"),
            end_widget=Gtk.Button(label="End"),
        )
        self.set_child(box)


def on_activate(app):
    # Create window
    win = CenterBoxWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
