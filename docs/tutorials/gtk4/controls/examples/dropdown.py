import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class DropDownWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="DropDown Demo")

        dropdown = Gtk.DropDown()
        dropdown.connect("notify::selected-item", self.on_string_selected)
        self.set_child(dropdown)

        strings = Gtk.StringList()
        dropdown.props.model = strings
        items = "This is a long list of words to populate the dropdown".split()

        # Populate the list
        for item in items:
            strings.append(item)

    def on_string_selected(self, dropdown, _pspec):
        # Selected Gtk.StringObject
        selected = dropdown.props.selected_item
        if selected is not None:
            print("Selected", selected.props.string)


def on_activate(app):
    win = DropDownWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
