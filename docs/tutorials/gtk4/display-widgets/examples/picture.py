import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class PictureWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Picture Demo")

        # Let's set a windows size so pictures don't start at their default size
        self.set_default_size(500, 400)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_child(box)

        label = Gtk.Label(label="Here we can see some nice pictures:")
        box.append(label)

        picture = Gtk.Picture.new_for_filename("../images/spinner_ext.png")
        box.append(picture)

        cover_picture = Gtk.Picture.new_for_filename("../images/spinner_ext.png")
        cover_picture.props.content_fit = Gtk.ContentFit.COVER
        box.append(cover_picture)


def on_activate(app):
    win = PictureWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
