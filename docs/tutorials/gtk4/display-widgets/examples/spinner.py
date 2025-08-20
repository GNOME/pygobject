import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class SpinnerAnimation(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Spinner Demo")

        button = Gtk.ToggleButton(label="Start Spinning")
        button.connect("toggled", self.on_button_toggled)
        button.props.active = False

        self.spinner = Gtk.Spinner()

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, homogeneous=True)
        box.append(button)
        box.append(self.spinner)
        self.set_child(box)

    def on_button_toggled(self, button):
        if button.props.active:
            self.spinner.start()
            button.set_label("Stop Spinning")

        else:
            self.spinner.stop()
            button.set_label("Start Spinning")


def on_activate(app):
    win = SpinnerAnimation(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
