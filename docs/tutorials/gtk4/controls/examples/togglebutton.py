import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class ToggleButtonWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="ToggleButton Demo")

        hbox = Gtk.Box(spacing=6)
        self.set_child(hbox)

        button = Gtk.ToggleButton(label="Button 1")
        button.connect("toggled", self.on_button_toggled, "1")
        hbox.append(button)

        button = Gtk.ToggleButton(label="B_utton 2", use_underline=True)
        button.set_active(True)
        button.connect("toggled", self.on_button_toggled, "2")
        hbox.append(button)

    def on_button_toggled(self, button, name):
        state = "on" if button.props.active else "off"
        print("Button", name, "was turned", state)


def on_activate(app):
    win = ToggleButtonWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
