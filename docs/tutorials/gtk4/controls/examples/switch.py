import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class SwitcherWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Switch Demo")

        hbox = Gtk.Box(spacing=6, homogeneous=True)
        hbox.props.margin_top = 24
        hbox.props.margin_bottom = 24
        self.set_child(hbox)

        switch = Gtk.Switch()
        switch.connect("notify::active", self.on_switch_activated)
        switch.props.active = False
        switch.props.halign = Gtk.Align.CENTER
        hbox.append(switch)

        switch = Gtk.Switch()
        switch.connect("notify::active", self.on_switch_activated)
        switch.props.active = True
        switch.props.halign = Gtk.Align.CENTER
        hbox.append(switch)

    def on_switch_activated(self, switch, _gparam):
        state = "on" if switch.props.active else "off"
        print("Switch was turned", state)


def on_activate(app):
    win = SwitcherWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
