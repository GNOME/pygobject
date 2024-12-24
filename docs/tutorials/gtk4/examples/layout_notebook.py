import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class NotebookWindow(Gtk.Window):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Simple Notebook Example")

        notebook = Gtk.Notebook()
        self.set_child(notebook)

        page1 = Gtk.Box()
        page1.append(Gtk.Label(label="Default Page!"))
        notebook.append_page(page1, Gtk.Label(label="Plain Title"))

        page2 = Gtk.Box()
        page2.append(Gtk.Label(label="A page with an image for a Title."))
        notebook.append_page(page2, Gtk.Image(icon_name="help-about"))


def on_activate(app):
    # Create window
    win = NotebookWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
