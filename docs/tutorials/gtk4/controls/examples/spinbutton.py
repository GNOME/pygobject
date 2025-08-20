import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class SpinButtonWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="SpinButton Demo")

        hbox = Gtk.Box(spacing=6)
        self.set_child(hbox)

        adjustment = Gtk.Adjustment(upper=100, step_increment=1, page_increment=10)
        self.spinbutton = Gtk.SpinButton()
        self.spinbutton.props.adjustment = adjustment
        self.spinbutton.connect("value-changed", self.on_value_changed)
        hbox.append(self.spinbutton)

        check_numeric = Gtk.CheckButton(label="Numeric")
        check_numeric.connect("toggled", self.on_numeric_toggled)
        hbox.append(check_numeric)

        check_ifvalid = Gtk.CheckButton(label="If Valid")
        check_ifvalid.connect("toggled", self.on_ifvalid_toggled)
        hbox.append(check_ifvalid)

    def on_value_changed(self, _scroll):
        print(self.spinbutton.get_value_as_int())

    def on_numeric_toggled(self, button):
        self.spinbutton.props.numeric = button.props.active

    def on_ifvalid_toggled(self, button):
        if button.get_active():
            policy = Gtk.SpinButtonUpdatePolicy.IF_VALID
        else:
            policy = Gtk.SpinButtonUpdatePolicy.ALWAYS
        self.spinbutton.props.update_policy = policy


def on_activate(app):
    win = SpinButtonWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
