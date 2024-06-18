import gi

gi.require_version('Gtk', '4.0')
from gi.repository import Gtk


class LinkButtonWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title='LinkButton Demo')

        button = Gtk.LinkButton(
            uri='https://www.gtk.org', label='Visit GTK Homepage'
        )
        self.set_child(button)


def on_activate(app):
    win = LinkButtonWindow(application=app)
    win.present()


app = Gtk.Application(application_id='com.example.App')
app.connect('activate', on_activate)

app.run(None)
