import sys
import gi

gi.require_version("Gtk", "4.0")
from gi.repository import GLib, Gio, Gtk

# This would typically be its own file
MENU_XML = """
<?xml version="1.0" encoding="UTF-8"?>
<interface>
<menu id="menubar">
    <submenu>
        <attribute name="label" translatable="yes">Change label</attribute>
        <section>
            <item>
                <attribute name="action">win.change_label</attribute>
                <attribute name="target">String 1</attribute>
                <attribute name="label" translatable="yes">String 1</attribute>
            </item>
            <item>
                <attribute name="action">win.change_label</attribute>
                <attribute name="target">String 2</attribute>
                <attribute name="label" translatable="yes">String 2</attribute>
            </item>
            <item>
                <attribute name="action">win.change_label</attribute>
                <attribute name="target">String 3</attribute>
                <attribute name="label" translatable="yes">String 3</attribute>
            </item>
        </section>
    </submenu>
    <submenu>
        <attribute name="label" translatable="yes">Window</attribute>
        <section>
            <item>
                <attribute name="action">win.maximize</attribute>
                <attribute name="label" translatable="yes">Maximize</attribute>
            </item>
        </section>
        <section>
            <item>
                <attribute name="action">app.about</attribute>
                <attribute name="label" translatable="yes">_About</attribute>
            </item>
            <item>
                <attribute name="action">app.quit</attribute>
                <attribute name="label" translatable="yes">_Quit</attribute>
                <attribute name="accel">&lt;Primary&gt;q</attribute>
            </item>
        </section>
    </submenu>
</menu>
</interface>
"""


class AppWindow(Gtk.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.set_default_size(200, 200)

        # By default the title bar will be hide, let's show it
        self.props.show_menubar = True

        # This will be in the windows group and have the 'win' prefix
        max_action = Gio.SimpleAction.new_stateful(
            "maximize", None, GLib.Variant.new_boolean(False)
        )
        max_action.connect("change-state", self.on_maximize_toggle)
        self.add_action(max_action)

        # Keep it in sync with the actual state
        self.connect(
            "notify::maximized",
            lambda obj, _pspec: max_action.set_state(
                GLib.Variant.new_boolean(obj.props.maximized)
            ),
        )

        lbl_variant = GLib.Variant.new_string("String 1")
        lbl_action = Gio.SimpleAction.new_stateful(
            "change_label", lbl_variant.get_type(), lbl_variant
        )
        lbl_action.connect("change-state", self.on_change_label_state)
        self.add_action(lbl_action)

        self.label = Gtk.Label(label=lbl_variant.get_string())
        self.set_child(self.label)

    def on_change_label_state(self, action, value):
        action.set_state(value)
        self.label.set_text(value.get_string())

    def on_maximize_toggle(self, action, value):
        action.set_state(value)
        if value.get_boolean():
            self.maximize()
        else:
            self.unmaximize()


class Application(Gtk.Application):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            application_id="org.example.App",
            flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE,
            **kwargs,
        )
        self.window = None

        self.add_main_option(
            "test",
            ord("t"),
            GLib.OptionFlags.NONE,
            GLib.OptionArg.NONE,
            "Command line test",
            None,
        )

    def do_startup(self):
        Gtk.Application.do_startup(self)

        action = Gio.SimpleAction.new("about", None)
        action.connect("activate", self.on_about)
        self.add_action(action)

        action = Gio.SimpleAction.new("quit", None)
        action.connect("activate", self.on_quit)
        self.add_action(action)

        builder = Gtk.Builder.new_from_string(MENU_XML, -1)
        self.set_menubar(builder.get_object("menubar"))

    def do_activate(self):
        # We only allow a single window and raise any existing ones
        if not self.window:
            # Windows are associated with the application
            # when the last one is closed the application shuts down
            self.window = AppWindow(application=self, title="Main Window")

        self.window.present()

    def do_command_line(self, command_line):
        options = command_line.get_options_dict()
        # convert GVariantDict -> GVariant -> dict
        options = options.end().unpack()

        if "test" in options:
            # This is printed on the main instance
            pass

        self.activate()
        return 0

    def on_about(self, _action, _param):
        about_dialog = Gtk.AboutDialog(transient_for=self.window, modal=True)
        about_dialog.present()

    def on_quit(self, _action, _param):
        self.quit()


if __name__ == "__main__":
    app = Application()
    app.run(sys.argv)
