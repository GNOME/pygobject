import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class ButtonWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Button Demo")

        hbox = Gtk.Box(spacing=6)
        self.set_child(hbox)

        button = Gtk.Button.new_with_label("Click Me")
        button.connect("clicked", self.on_click_me_clicked)
        hbox.append(button)

        button = Gtk.Button.new_with_mnemonic("_Open")
        button.connect("clicked", self.on_open_clicked)
        hbox.append(button)

        button = Gtk.Button.new_with_mnemonic("_Close")
        button.connect("clicked", self.on_close_clicked)
        hbox.append(button)

    def on_click_me_clicked(self, _button):
        print("[Click me] button was clicked")

    def on_open_clicked(self, _button):
        print("[Open] button was clicked")

    def on_close_clicked(self, _button):
        print("Closing application")
        self.close()


def on_activate(app):
    win = ButtonWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
