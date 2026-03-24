import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk


class LabelWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="Label Demo")

        box = Gtk.Box(spacing=10)
        self.set_child(box)

        box_left = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=10,
            hexpand=True,
            homogeneous=True,
        )
        box_right = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)

        box.append(box_left)
        box.append(box_right)

        label = Gtk.Label(label="This is a normal label")
        box_left.append(label)

        label = Gtk.Label(
            label="This is a normal label with xalign set to 0",
            xalign=0,
        )
        box_left.append(label)

        label = Gtk.Label(
            label="This is a left-justified label.\nWith multiple lines.",
            justify=Gtk.Justification.LEFT,
        )
        box_left.append(label)

        label = Gtk.Label(
            label="This is a right-justified label.\nWith multiple lines.",
            justify=Gtk.Justification.RIGHT,
        )
        box_left.append(label)

        label = Gtk.Label(
            label="This is an example of a line-wrapped label.  It "
            "should not be taking up the entire             "
            "width allocated to it, but automatically "
            "wraps the words to fit.\n"
            "     It supports multiple paragraphs correctly, "
            "and  correctly   adds "
            "many          extra  spaces. ",
            wrap=True,
            max_width_chars=32,
        )
        box_right.append(label)

        label = Gtk.Label(
            label="This is an example of a line-wrapped, filled label. "
            "It should be taking "
            "up the entire              width allocated to it.  "
            "Here is a sentence to prove "
            "my point.  Here is another sentence. "
            "Here comes the sun, do de do de do.\n"
            "    This is a new paragraph.\n"
            "    This is another newer, longer, better "
            "paragraph.  It is coming to an end, "
            "unfortunately.",
            wrap=True,
            justify=Gtk.Justification.FILL,
            max_width_chars=32,
        )
        box_right.append(label)

        label = Gtk.Label(wrap=True, max_width_chars=48)
        label.set_markup(
            "Text can be <small>small</small>, <big>big</big>, "
            "<b>bold</b>, <i>italic</i> and even point to "
            'somewhere in the <a href="https://www.gtk.org" '
            'title="Click to find out more">internets</a>.'
        )
        box_left.append(label)

        button = Gtk.Button(label="Click at your own risk")
        label = Gtk.Label(
            label="_Press Alt + P to select button to the right",
            use_underline=True,
            selectable=True,
            mnemonic_widget=button,
        )
        box_left.append(label)
        box_right.append(button)


def on_activate(app):
    win = LabelWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
