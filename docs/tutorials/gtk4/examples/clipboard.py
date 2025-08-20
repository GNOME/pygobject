import gi

gi.require_version("Gdk", "4.0")
gi.require_version("Gtk", "4.0")
from gi.repository import Gdk, Gtk


class ClipboardWindow(Gtk.ApplicationWindow):
    def __init__(self, *args, **kargs):
        super().__init__(*args, **kargs, title="Clipboard Example")

        box = Gtk.Box(spacing=12, orientation=Gtk.Orientation.VERTICAL)
        self.set_child(box)

        self.clipboard = Gdk.Display.get_default().get_clipboard()

        text_box = Gtk.Box(spacing=6, homogeneous=True)
        box.append(text_box)

        self.entry = Gtk.Entry(text="Some text you can copy")
        button_copy_text = Gtk.Button(label="Copy Text")
        button_copy_text.connect("clicked", self.copy_text)
        button_paste_text = Gtk.Button(label="Paste Text")
        button_paste_text.connect("clicked", self.paste_text)

        text_box.append(self.entry)
        text_box.append(button_copy_text)
        text_box.append(button_paste_text)

        image_box = Gtk.Box(spacing=6)
        box.append(image_box)

        self.picture = Gtk.Picture.new_for_filename("../images/application.png")
        self.picture.props.hexpand = True
        button_copy_image = Gtk.Button(label="Copy Image", valign=Gtk.Align.CENTER)
        button_copy_image.connect("clicked", self.copy_image)
        button_paste_image = Gtk.Button(label="Paste Image", valign=Gtk.Align.CENTER)
        button_paste_image.connect("clicked", self.paste_image)

        image_box.append(self.picture)
        image_box.append(button_copy_image)
        image_box.append(button_paste_image)

    def copy_text(self, _button):
        self.clipboard.set(self.entry.get_text())

    def paste_text(self, _button):
        self.clipboard.read_text_async(None, self.on_paste_text)

    def on_paste_text(self, _clipboard, result):
        text = self.clipboard.read_text_finish(result)
        if text is not None:
            self.entry.set_text(text)

    def copy_image(self, _button):
        texture = self.picture.get_paintable()
        gbytes = texture.save_to_png_bytes()
        content = Gdk.ContentProvider.new_for_bytes("image/png", gbytes)
        self.clipboard.set_content(content)

    def paste_image(self, _button):
        self.clipboard.read_texture_async(None, self.on_paste_image)

    def on_paste_image(self, _clipboard, result):
        texture = self.clipboard.read_texture_finish(result)
        if texture is not None:
            self.picture.set_paintable(texture)


def on_activate(app):
    win = ClipboardWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
