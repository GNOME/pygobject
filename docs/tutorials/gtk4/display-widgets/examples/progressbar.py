import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, GLib


class ProgressBarWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="ProgressBar Demo")

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_child(vbox)

        self.progressbar = Gtk.ProgressBar()
        vbox.append(self.progressbar)

        button = Gtk.CheckButton(label="Show text")
        button.connect("toggled", self.on_show_text_toggled)
        vbox.append(button)

        button = Gtk.CheckButton(label="Activity mode")
        button.connect("toggled", self.on_activity_mode_toggled)
        vbox.append(button)

        button = Gtk.CheckButton(label="Right to Left")
        button.connect("toggled", self.on_right_to_left_toggled)
        vbox.append(button)

        self.timeout_id = GLib.timeout_add(50, self.on_timeout)
        self.activity_mode = False

    def on_show_text_toggled(self, button):
        show_text = button.props.active
        text = "some text" if show_text else None
        self.progressbar.props.text = text
        self.progressbar.props.show_text = show_text

    def on_activity_mode_toggled(self, button):
        self.activity_mode = button.props.active
        if self.activity_mode:
            self.progressbar.pulse()
        else:
            self.progressbar.props.fraction = 0.0

    def on_right_to_left_toggled(self, button):
        value = button.props.active
        self.progressbar.props.inverted = value

    def on_timeout(self):
        """Update value on the progress bar."""
        if self.activity_mode:
            self.progressbar.pulse()
        else:
            new_value = self.progressbar.props.fraction + 0.01

            if new_value > 1:
                new_value = 0

            self.progressbar.props.fraction = new_value

        # As this is a timeout function, return True so that it
        # continues to get called
        return True


def on_activate(app):
    win = ProgressBarWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
