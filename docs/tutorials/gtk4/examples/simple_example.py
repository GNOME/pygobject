import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


def on_activate(app):
    # Create window
    win = Gtk.ApplicationWindow(application=app)
    win.present()


# Create a new application
app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

# Run the application
app.run(None)
