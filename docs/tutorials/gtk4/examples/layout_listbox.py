import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class ListBoxRowWithData(Gtk.ListBoxRow):
    def __init__(self, data):
        super().__init__()
        self.data = data
        self.set_child(Gtk.Label(label=data))


class ListBoxWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, default_width=400, title="ListBox Demo")

        # Main box of out window
        box_outer = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=24)
        box_outer.props.margin_start = 24
        box_outer.props.margin_end = 24
        box_outer.props.margin_top = 24
        box_outer.props.margin_bottom = 24
        self.set_child(box_outer)

        # Let's create our first ListBox
        listbox = Gtk.ListBox()
        listbox.props.selection_mode = Gtk.SelectionMode.NONE
        listbox.props.show_separators = True
        box_outer.append(listbox)

        # Let's create our first ListBoxRow
        row = Gtk.ListBoxRow()
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=24)
        row.set_child(hbox)  # We set the Box as the ListBoxRow child
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        hbox.append(vbox)
        label1 = Gtk.Label(label="Automatic Date & Time", xalign=0)
        label2 = Gtk.Label(label="Requires internet access", xalign=0)
        vbox.append(label1)
        vbox.append(label2)

        switch = Gtk.Switch()
        switch.props.hexpand = True  # Lets make the Switch expand to the window width
        switch.props.halign = Gtk.Align.END  # Horizontally aligned to the end
        switch.props.valign = Gtk.Align.CENTER  # Vertically aligned to the center
        hbox.append(switch)

        listbox.append(row)  # Add the row to the list

        # Our second row. We will omit the ListBoxRow and directly append a Box
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=24)
        label = Gtk.Label(label="Enable Automatic Update", xalign=0)
        check = Gtk.CheckButton()
        check.props.hexpand = True
        check.props.halign = Gtk.Align.END
        hbox.append(label)
        hbox.append(check)
        listbox.append(hbox)  # Add the second row to the list

        # Let's create a second ListBox
        listbox_2 = Gtk.ListBox()
        box_outer.append(listbox_2)
        items = ["This", "is", "a", "sorted", "ListBox", "Fail"]

        # Populate the list
        for item in items:
            listbox_2.append(ListBoxRowWithData(item))

        # Set sorting and filter functions
        listbox_2.set_sort_func(self.sort_func)
        listbox_2.set_filter_func(self.filter_func)

        # Connect to "row-activated" signal
        listbox_2.connect("row-activated", self.on_row_activated)

    def sort_func(self, row_1, row_2):
        return row_1.data.lower() > row_2.data.lower()

    def filter_func(self, row):
        return row.data != "Fail"

    def on_row_activated(self, _listbox, row):
        pass


def on_activate(app):
    # Create window
    win = ListBoxWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
