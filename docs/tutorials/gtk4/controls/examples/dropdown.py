import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class DropDownWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="DropDown Demo")

        items = "This is a long list of words to populate the dropdown".split()
        dropdown = Gtk.DropDown.new_from_strings(items)
        dropdown.connect("notify::selected-item", self.on_string_selected)
        self.set_child(dropdown)

    def on_string_selected(self, dropdown, _pspec):
        # Selected Gtk.StringObject
        if selected := dropdown.get_selected_item():
            print("Selected", selected.get_string())


def on_activate(app):
    win = DropDownWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
