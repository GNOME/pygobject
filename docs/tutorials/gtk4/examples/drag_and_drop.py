import gi

gi.require_version("Gdk", "4.0")
gi.require_version("Gtk", "4.0")
from gi.repository import Gdk, GObject, Gtk


class DragDropWindow(Gtk.ApplicationWindow):
    def __init__(self, *args, **kargs):
        super().__init__(*args, **kargs, title="Drag and Drop Example")

        self.set_default_size(500, 400)

        views_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        views_box.props.vexpand = True
        self.set_child(views_box)

        flow_box = Gtk.FlowBox()
        views_box.append(flow_box)
        flow_box.props.selection_mode = Gtk.SelectionMode.NONE
        flow_box.append(SourceFlowBoxChild("Item 1", "image-missing"))
        flow_box.append(SourceFlowBoxChild("Item 2", "help-about"))
        flow_box.append(SourceFlowBoxChild("Item 3", "edit-copy"))

        views_box.append(Gtk.Separator())

        self.target_view = TargetView(vexpand=True)
        views_box.append(self.target_view)


class SourceFlowBoxChild(Gtk.FlowBoxChild):
    def __init__(self, name, icon_name):
        super().__init__()

        self.name = name
        self.icon_name = icon_name

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_child(box)

        icon = Gtk.Image(icon_name=self.icon_name)
        label = Gtk.Label(label=self.name)

        box.append(icon)
        box.append(label)

        drag_controller = Gtk.DragSource()
        drag_controller.connect("prepare", self.on_drag_prepare)
        drag_controller.connect("drag-begin", self.on_drag_begin)
        self.add_controller(drag_controller)

    def on_drag_prepare(self, _ctrl, _x, _y):
        item = Gdk.ContentProvider.new_for_value(self)
        string = Gdk.ContentProvider.new_for_value(self.name)
        return Gdk.ContentProvider.new_union([item, string])

    def on_drag_begin(self, ctrl, _drag):
        icon = Gtk.WidgetPaintable.new(self)
        ctrl.set_icon(icon, 0, 0)


class TargetView(Gtk.Box):
    def __init__(self, **kargs):
        super().__init__(**kargs)

        self.stack = Gtk.Stack(hexpand=True)
        self.append(self.stack)

        empty_label = Gtk.Label(label="Drag some item, text, or files here.")
        self.stack.add_named(empty_label, "empty")
        self.stack.set_visible_child_name("empty")

        box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            vexpand=True,
            valign=Gtk.Align.CENTER,
        )
        self.stack.add_named(box, "item")

        self.icon = Gtk.Image()
        box.append(self.icon)
        self.label = Gtk.Label()
        box.append(self.label)

        self.text = Gtk.Label()
        self.stack.add_named(self.text, "other")

        drop_controller = Gtk.DropTarget.new(
            type=GObject.TYPE_NONE, actions=Gdk.DragAction.COPY
        )
        drop_controller.set_gtypes([SourceFlowBoxChild, Gdk.FileList, str])
        drop_controller.connect("drop", self.on_drop)
        self.add_controller(drop_controller)

    def on_drop(self, _ctrl, value, _x, _y):
        if isinstance(value, SourceFlowBoxChild):
            self.label.props.label = value.name
            self.icon.props.icon_name = value.icon_name
            self.stack.set_visible_child_name("item")

        elif isinstance(value, Gdk.FileList):
            files = value.get_files()
            names = ""
            for file in files:
                names += f"Loaded file {file.get_basename()}\n"
            self.text.props.label = names
            self.stack.set_visible_child_name("other")

        elif isinstance(value, str):
            self.text.props.label = value
            self.stack.set_visible_child_name("other")


def on_activate(app):
    win = DragDropWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
