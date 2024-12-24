import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gio, Gtk

# This would typically be its own file
MENU_XML = """
<?xml version='1.0' encoding='UTF-8'?>
<interface>
  <menu id='app-menu'>
    <section>
        <item>
            <attribute name='label'>About</attribute>
            <attribute name='action'>app.about</attribute>
        </item>
        <item>
            <attribute name='label'>Quit</attribute>
            <attribute name='action'>app.quit</attribute>
        </item>
    </section>
  </menu>
</interface>
"""


class AppWindow(Gtk.ApplicationWindow):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.set_default_size(300, 200)

        headerbar = Gtk.HeaderBar()
        self.set_titlebar(headerbar)

        builder = Gtk.Builder.new_from_string(MENU_XML, -1)
        menu_model = builder.get_object("app-menu")

        button = Gtk.MenuButton(menu_model=menu_model)
        headerbar.pack_end(button)


class Application(Gtk.Application):
    def __init__(self, **kwargs):
        super().__init__(application_id="com.example.App", **kwargs)
        self.window = None

    def do_startup(self):
        Gtk.Application.do_startup(self)

        action = Gio.SimpleAction(name="about")
        action.connect("activate", self.on_about)
        self.add_action(action)

        action = Gio.SimpleAction(name="quit")
        action.connect("activate", self.on_quit)
        self.add_action(action)

    def do_activate(self):
        # We only allow a single window and raise any existing ones
        if not self.window:
            # Windows are associated with the application
            # when the last one is closed the application shuts down
            self.window = AppWindow(application=self, title="Main Window")

        self.window.present()

    def on_about(self, action, param):
        about_dialog = Gtk.AboutDialog(transient_for=self.window, modal=True)
        about_dialog.present()

    def on_quit(self, action, param):
        self.quit()


app = Application()
app.run(None)
