import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, Gdk


class FlowBoxWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="FlowBox Demo")

        self.set_default_size(300, 250)

        header = Gtk.HeaderBar()
        self.set_titlebar(header)

        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.set_child(scrolled)

        flowbox = Gtk.FlowBox()
        flowbox.props.valign = Gtk.Align.START
        flowbox.props.max_children_per_line = 30
        flowbox.props.selection_mode = Gtk.SelectionMode.NONE
        scrolled.set_child(flowbox)

        self.create_flowbox(flowbox)

    def draw_button_color(self, area, cr, width, height, rgba):
        context = area.get_style_context()

        Gtk.render_background(context, cr, 0, 0, width, height)

        cr.set_source_rgba(rgba.red, rgba.green, rgba.blue, rgba.alpha)
        cr.rectangle(0, 0, width, height)
        cr.fill()

    def color_swatch_new(self, str_color):
        rgba = Gdk.RGBA()
        rgba.parse(str_color)

        button = Gtk.Button()

        area = Gtk.DrawingArea()
        area.set_size_request(24, 24)
        area.set_draw_func(self.draw_button_color, rgba)

        button.set_child(area)

        return button

    def create_flowbox(self, flowbox):
        colors = [
            "AliceBlue",
            "AntiqueWhite",
            "AntiqueWhite1",
            "AntiqueWhite2",
            "AntiqueWhite3",
            "AntiqueWhite4",
            "aqua",
            "aquamarine",
            "aquamarine1",
            "aquamarine2",
            "aquamarine3",
            "aquamarine4",
            "azure",
            "azure1",
            "azure2",
            "azure3",
            "azure4",
            "beige",
            "bisque",
            "bisque1",
            "bisque2",
            "bisque3",
            "bisque4",
            "black",
            "BlanchedAlmond",
            "blue",
            "blue1",
            "blue2",
            "blue3",
            "blue4",
            "BlueViolet",
            "brown",
            "brown1",
            "brown2",
            "brown3",
            "brown4",
            "burlywood",
            "burlywood1",
            "burlywood2",
            "burlywood3",
            "burlywood4",
            "CadetBlue",
            "CadetBlue1",
            "CadetBlue2",
            "CadetBlue3",
            "CadetBlue4",
            "chartreuse",
            "chartreuse1",
            "chartreuse2",
            "chartreuse3",
            "chartreuse4",
            "chocolate",
            "chocolate1",
            "chocolate2",
            "chocolate3",
            "chocolate4",
            "coral",
            "coral1",
            "coral2",
            "coral3",
            "coral4",
        ]

        for color in colors:
            button = self.color_swatch_new(color)
            button.props.tooltip_text = color
            flowbox.append(button)


def on_activate(app):
    # Create window
    win = FlowBoxWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
