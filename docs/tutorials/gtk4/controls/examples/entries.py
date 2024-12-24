import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, GLib


class EntryWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Entry Demo")

        self.set_size_request(200, 100)

        self.timeout_id = None

        header = Gtk.HeaderBar()
        self.set_titlebar(header)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        vbox.props.margin_start = 24
        vbox.props.margin_end = 24
        vbox.props.margin_top = 24
        vbox.props.margin_bottom = 24
        self.set_child(vbox)

        # Gtk.SearchEntry
        search = Gtk.SearchEntry()
        search.props.placeholder_text = "Search Entry"
        search.set_key_capture_widget(self)
        header.set_title_widget(search)
        self.set_focus(search)

        # Gtk.Entry
        self.entry = Gtk.Entry()
        self.entry.set_text("Hello World")
        vbox.append(self.entry)

        hbox = Gtk.Box(spacing=6)
        vbox.append(hbox)

        self.check_editable = Gtk.CheckButton(label="Editable")
        self.check_editable.connect("toggled", self.on_editable_toggled)
        self.check_editable.props.active = True
        hbox.append(self.check_editable)

        self.check_visible = Gtk.CheckButton(label="Visible")
        self.check_visible.connect("toggled", self.on_visible_toggled)
        self.check_visible.props.active = True
        hbox.append(self.check_visible)

        self.pulse = Gtk.CheckButton(label="Pulse")
        self.pulse.connect("toggled", self.on_pulse_toggled)
        hbox.append(self.pulse)

        self.icon = Gtk.CheckButton(label="Icon")
        self.icon.connect("toggled", self.on_icon_toggled)
        hbox.append(self.icon)

        # Gtk.PasswordEntry
        pass_entry = Gtk.PasswordEntry()
        pass_entry.props.placeholder_text = "Password Entry"
        pass_entry.props.show_peek_icon = True
        pass_entry.props.margin_top = 24
        vbox.append(pass_entry)

    def on_editable_toggled(self, button):
        value = button.get_active()
        self.entry.set_editable(value)

    def on_visible_toggled(self, button):
        self.entry.props.visibility = button.props.active

    def on_pulse_toggled(self, button):
        if button.get_active():
            self.entry.props.progress_pulse_step = 0.2
            # Call self.do_pulse every 100 ms
            self.timeout_id = GLib.timeout_add(100, self.do_pulse)
        else:
            # Don't call self.do_pulse anymore
            GLib.source_remove(self.timeout_id)
            self.timeout_id = None
            self.entry.props.progress_pulse_step = 0

    def do_pulse(self):
        self.entry.progress_pulse()
        return True

    def on_icon_toggled(self, button):
        icon_name = "system-search-symbolic" if button.props.active else None
        self.entry.set_icon_from_icon_name(Gtk.EntryIconPosition.PRIMARY, icon_name)


def on_activate(app):
    win = EntryWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
