import asyncio
import time
import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gio, GLib, Gtk
from gi.events import GLibEventLoopPolicy


class DownloadWindow(Gtk.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            **kwargs,
            default_width=500,
            default_height=400,
            title="Async I/O Example",
        )

        self.background_task = None

        self.cancel_button = Gtk.Button(label="Cancel")
        self.cancel_button.connect("clicked", self.on_cancel_clicked)
        self.cancel_button.set_sensitive(False)

        self.start_button = Gtk.Button(label="Load")
        self.start_button.connect("clicked", self.on_start_clicked)

        textview = Gtk.TextView(vexpand=True)
        self.textbuffer = textview.get_buffer()
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_child(textview)

        box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=6,
            margin_start=12,
            margin_end=12,
            margin_top=12,
            margin_bottom=12,
        )
        box.append(self.start_button)
        box.append(self.cancel_button)
        box.append(scrolled)

        self.set_child(box)

    def append_text(self, text):
        iter_ = self.textbuffer.get_end_iter()
        self.textbuffer.insert(iter_, f"[{time.time()}] {text}\n")

    def on_start_clicked(self, button):
        button.set_sensitive(False)
        self.cancel_button.set_sensitive(True)
        self.append_text("Start clicked...")

        self.background_task = asyncio.create_task(self.download())

    def on_cancel_clicked(self, button):
        self.append_text("Cancel clicked...")
        self.background_task.cancel()

    async def download(self):
        file_ = Gio.File.new_for_uri("https://pygobject.gnome.org/")

        try:
            _success, content, _etag = await file_.load_contents_async()
        except GLib.GError as e:
            self.append_text(f"Error: {e.message}")
        else:
            content_text = content[:100].decode("utf-8")
            self.append_text(f"Got content: {content_text}...")
        finally:
            self.cancel_button.set_sensitive(False)
            self.start_button.set_sensitive(True)


class Application(Gtk.Application):
    def do_activate(self):
        window = DownloadWindow(application=self)
        window.present()


def main():
    asyncio.set_event_loop_policy(GLibEventLoopPolicy())
    app = Application()
    app.run()


if __name__ == "__main__":
    main()
