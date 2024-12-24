import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, GObject


class StackWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Stack Demo")

        self.set_default_size(300, 250)

        header = Gtk.HeaderBar()
        self.set_titlebar(header)

        stack = Gtk.Stack()
        stack.props.transition_type = Gtk.StackTransitionType.SLIDE_LEFT_RIGHT
        stack.props.transition_duration = 1000
        self.set_child(stack)

        checkbutton = Gtk.CheckButton(label="Click me!")
        checkbutton.props.hexpand = True
        checkbutton.props.halign = Gtk.Align.CENTER
        page1 = stack.add_titled(checkbutton, "check", "Check Button")
        checkbutton.bind_property(
            "active", page1, "needs-attention", GObject.BindingFlags.DEFAULT
        )

        label = Gtk.Label()
        label.set_markup("<big>A fancy label</big>")
        stack.add_titled(label, "label", "A label")

        stack_switcher = Gtk.StackSwitcher()
        stack_switcher.set_stack(stack)
        header.set_title_widget(stack_switcher)

        # Let's start in the second page
        stack.set_visible_child_name("label")


def on_activate(app):
    # Create window
    win = StackWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
