import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class HeaderBarWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, default_width=400, title="HeaderBar Example")

        header_bar = Gtk.HeaderBar()
        self.set_titlebar(header_bar)

        button = Gtk.Button(label="Button")
        header_bar.pack_start(button)

        icon_button = Gtk.Button(icon_name="open-menu-symbolic")
        header_bar.pack_end(icon_button)


def on_activate(app):
    # Create window
    win = HeaderBarWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
