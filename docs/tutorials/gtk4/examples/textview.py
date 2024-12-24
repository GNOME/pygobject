import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, Pango


class SearchDialog(Gtk.Window):
    def __init__(self, parent):
        super().__init__(title="Search", transient_for=parent)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        self.set_child(box)

        label = Gtk.Label(label="Insert text you want to search for:")
        box.append(label)

        self.entry = Gtk.Entry()
        box.append(self.entry)

        self.button = Gtk.Button(label="Find")
        box.append(self.button)


class TextViewWindow(Gtk.ApplicationWindow):
    def __init__(self, **kargs):
        super().__init__(**kargs, title="TextView Demo")

        self.set_default_size(500, 400)

        self.box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_child(self.box)

        self.create_textview()
        self.create_toolbar()
        self.create_buttons()

    def create_toolbar(self):
        toolbar = Gtk.Box(spacing=6)
        toolbar.props.margin_top = 6
        toolbar.props.margin_start = 6
        toolbar.props.margin_end = 6
        self.box.prepend(toolbar)

        button_bold = Gtk.Button(icon_name="format-text-bold-symbolic")
        toolbar.append(button_bold)

        button_italic = Gtk.Button(icon_name="format-text-italic-symbolic")
        toolbar.append(button_italic)

        button_underline = Gtk.Button(icon_name="format-text-underline-symbolic")
        toolbar.append(button_underline)

        button_bold.connect("clicked", self.on_button_clicked, self.tag_bold)
        button_italic.connect("clicked", self.on_button_clicked, self.tag_italic)
        button_underline.connect("clicked", self.on_button_clicked, self.tag_underline)

        toolbar.append(Gtk.Separator())

        justifyleft = Gtk.ToggleButton(icon_name="format-justify-left-symbolic")
        toolbar.append(justifyleft)

        justifycenter = Gtk.ToggleButton(icon_name="format-justify-center-symbolic")
        justifycenter.set_group(justifyleft)
        toolbar.append(justifycenter)

        justifyright = Gtk.ToggleButton(icon_name="format-justify-right-symbolic")
        justifyright.set_group(justifyleft)
        toolbar.append(justifyright)

        justifyfill = Gtk.ToggleButton(icon_name="format-justify-fill-symbolic")
        justifyfill.set_group(justifyleft)
        toolbar.append(justifyfill)

        justifyleft.connect("toggled", self.on_justify_toggled, Gtk.Justification.LEFT)
        justifycenter.connect(
            "toggled", self.on_justify_toggled, Gtk.Justification.CENTER
        )
        justifyright.connect(
            "toggled", self.on_justify_toggled, Gtk.Justification.RIGHT
        )
        justifyfill.connect("toggled", self.on_justify_toggled, Gtk.Justification.FILL)

        toolbar.append(Gtk.Separator())

        button_clear = Gtk.Button(icon_name="edit-clear-symbolic")
        button_clear.connect("clicked", self.on_clear_clicked)
        toolbar.append(button_clear)

        toolbar.append(Gtk.Separator())

        button_search = Gtk.Button(icon_name="system-search-symbolic")
        button_search.connect("clicked", self.on_search_clicked)
        toolbar.append(button_search)

    def create_textview(self):
        scrolledwindow = Gtk.ScrolledWindow()
        scrolledwindow.props.hexpand = True
        scrolledwindow.props.vexpand = True
        self.box.append(scrolledwindow)

        self.textview = Gtk.TextView()
        self.textbuffer = self.textview.get_buffer()
        self.textbuffer.set_text(
            "This is some text inside of a Gtk.TextView. "
            + 'Select text and click one of the buttons "bold", "italic", '
            + 'or "underline" to modify the text accordingly.'
        )
        scrolledwindow.set_child(self.textview)

        self.tag_bold = self.textbuffer.create_tag("bold", weight=Pango.Weight.BOLD)
        self.tag_italic = self.textbuffer.create_tag("italic", style=Pango.Style.ITALIC)
        self.tag_underline = self.textbuffer.create_tag(
            "underline", underline=Pango.Underline.SINGLE
        )
        self.tag_found = self.textbuffer.create_tag("found", background="yellow")

    def create_buttons(self):
        grid = Gtk.Grid()
        self.box.append(grid)

        check_editable = Gtk.CheckButton(label="Editable")
        check_editable.props.active = True
        check_editable.connect("toggled", self.on_editable_toggled)
        grid.attach(check_editable, 0, 0, 1, 1)

        check_cursor = Gtk.CheckButton(label="Cursor Visible")
        check_cursor.props.active = True
        check_editable.connect("toggled", self.on_cursor_toggled)
        grid.attach_next_to(check_cursor, check_editable, Gtk.PositionType.RIGHT, 1, 1)

        radio_wrapnone = Gtk.CheckButton(label="No Wrapping")
        radio_wrapnone.props.active = True
        grid.attach(radio_wrapnone, 0, 1, 1, 1)

        radio_wrapchar = Gtk.CheckButton(label="Character Wrapping")
        radio_wrapchar.set_group(radio_wrapnone)
        grid.attach_next_to(
            radio_wrapchar, radio_wrapnone, Gtk.PositionType.RIGHT, 1, 1
        )

        radio_wrapword = Gtk.CheckButton(label="Word Wrapping")
        radio_wrapword.set_group(radio_wrapnone)
        grid.attach_next_to(
            radio_wrapword, radio_wrapchar, Gtk.PositionType.RIGHT, 1, 1
        )

        radio_wrapnone.connect("toggled", self.on_wrap_toggled, Gtk.WrapMode.NONE)
        radio_wrapchar.connect("toggled", self.on_wrap_toggled, Gtk.WrapMode.CHAR)
        radio_wrapword.connect("toggled", self.on_wrap_toggled, Gtk.WrapMode.WORD)

    def on_button_clicked(self, _widget, tag):
        bounds = self.textbuffer.get_selection_bounds()
        if len(bounds) != 0:
            start, end = bounds
            self.textbuffer.apply_tag(tag, start, end)

    def on_clear_clicked(self, _widget):
        start = self.textbuffer.get_start_iter()
        end = self.textbuffer.get_end_iter()
        self.textbuffer.remove_all_tags(start, end)

    def on_editable_toggled(self, widget):
        self.textview.props.editable = widget.props.active

    def on_cursor_toggled(self, widget):
        self.textview.props.cursor_visible = widget.props.active

    def on_wrap_toggled(self, _widget, mode):
        self.textview.props.wrap_mode = mode

    def on_justify_toggled(self, _widget, justification):
        self.textview.props.justification = justification

    def on_search_clicked(self, _widget):
        self.search_dialog = SearchDialog(self)
        self.search_dialog.button.connect("clicked", self.on_find_clicked)
        self.search_dialog.present()

    def on_find_clicked(self, _button):
        cursor_mark = self.textbuffer.get_insert()
        start = self.textbuffer.get_iter_at_mark(cursor_mark)
        if start.get_offset() == self.textbuffer.get_char_count():
            start = self.textbuffer.get_start_iter()

        self.search_and_mark(self.search_dialog.entry.get_text(), start)

    def search_and_mark(self, text, start):
        end = self.textbuffer.get_end_iter()
        match = start.forward_search(text, 0, end)

        if match is not None:
            match_start, match_end = match
            self.textbuffer.apply_tag(self.tag_found, match_start, match_end)
            self.search_and_mark(text, match_end)


def on_activate(app):
    win = TextViewWindow(application=app)
    win.present()


app = Gtk.Application(application_id="com.example.App")
app.connect("activate", on_activate)

app.run(None)
