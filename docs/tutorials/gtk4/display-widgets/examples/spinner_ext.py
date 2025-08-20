import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, GLib


class SpinnerAnimation(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Spinner Demo")

        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_child(main_box)

        self.spinner = Gtk.Spinner()
        main_box.append(self.spinner)

        self.label = Gtk.Label()
        main_box.append(self.label)

        self.entry = Gtk.Entry()
        self.entry.props.text = "10"
        main_box.append(self.entry)

        self.button_start = Gtk.Button(label="Start timer")
        self.button_start.connect("clicked", self.on_button_start_clicked)
        main_box.append(self.button_start)

        self.button_stop = Gtk.Button(label="Stop timer")
        self.button_stop.props.sensitive = False
        self.button_stop.connect("clicked", self.on_button_stop_clicked)
        main_box.append(self.button_stop)

        self.timeout_id = None
        self.connect("unrealize", self.on_window_destroy)

    def on_button_start_clicked(self, _widget):
        """Handles 'clicked' event of button_start."""
        self.start_timer()

    def on_button_stop_clicked(self, _widget):
        """Handles 'clicked' event of button_stop."""
        self.stop_timer("Stopped from button")

    def on_window_destroy(self, _widget):
        """Handles unrealize event of main window."""
        # Ensure the timeout function is stopped
        if self.timeout_id:
            GLib.source_remove(self.timeout_id)
            self.timeout_id = None

    def on_timeout(self):
        """A timeout function.
        Return True to stop it.
        This is not a precise timer since next timeout
        is recalculated based on the current time.
        """
        self.counter -= 1
        if self.counter <= 0:
            self.stop_timer("Reached time out")
            return False
        self.label.props.label = f"Remaining: {int(self.counter / 4)!s}"
        return True

    def start_timer(self):
        """Start the timer."""
        self.button_start.props.sensitive = False
        self.button_stop.props.sensitive = True
        # time out will check every 250 milliseconds (1/4 of a second)
        self.counter = 4 * int(self.entry.props.text)
        self.label.props.label = f"Remaining: {int(self.counter / 4)!s}"
        self.spinner.start()
        self.timeout_id = GLib.timeout_add(250, self.on_timeout)

    def stop_timer(self, label_text):
        """Stop the timer."""
        if self.timeout_id:
            GLib.source_remove(self.timeout_id)
            self.timeout_id = None
        self.spinner.stop()
        self.button_start.props.sensitive = True
        self.button_stop.props.sensitive = False
        self.label.props.label = label_text


def on_activate(app):
    win = SpinnerAnimation(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
