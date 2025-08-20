import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class CheckButtonWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="CheckButton Demo")

        box = Gtk.Box(spacing=6)
        self.set_child(box)

        check = Gtk.CheckButton(label="Checkbox")
        check.connect("toggled", self.on_check_toggled)
        box.append(check)

        radio1 = Gtk.CheckButton(label="Radio 1")
        radio1.connect("toggled", self.on_radio_toggled, "1")
        box.append(radio1)

        radio2 = Gtk.CheckButton(label="Radio 2")
        radio2.set_group(radio1)
        radio2.connect("toggled", self.on_radio_toggled, "2")
        box.append(radio2)

        radio3 = Gtk.CheckButton.new_with_mnemonic("R_adio 3")
        radio3.set_group(radio1)
        radio3.connect("toggled", self.on_radio_toggled, "3")
        box.append(radio3)

    def on_check_toggled(self, check):
        state = "on" if check.props.active else "off"
        print("Checkbox was turned", state)

    def on_radio_toggled(self, radio, name):
        state = "on" if radio.props.active else "off"
        print("Radio", name, "was turned", state)


def on_activate(app):
    win = CheckButtonWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
