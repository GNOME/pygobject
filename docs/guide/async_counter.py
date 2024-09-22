import asyncio
import traceback

import gi

gi.require_version("Gtk", "4.0")

from gi.events import GLibEventLoopPolicy
from gi.repository import Gtk

background_tasks = set()


def _task_done_cb(task):
    if not task.cancelled() and (exc := task.exception()) is not None:
        traceback.print_exception(exc)
    background_tasks.discard(task)


def create_background_task(coro):
    task = asyncio.create_task(coro)
    background_tasks.add(task)
    task.add_done_callback(_task_done_cb)
    return task


async def update_label(label):
    i = 0
    while True:
        label.set_label(str(i))
        await asyncio.sleep(1)
        i += 1


def on_activate(app):
    win = Gtk.ApplicationWindow.new(app)
    win.set_default_size(300, 200)
    label = Gtk.Label.new("-")
    win.set_child(label)

    t = create_background_task(update_label(label))

    def do_unmap(win):
        t.cancel()

    win.connect("unmap", do_unmap)
    win.present()


def main():
    asyncio.set_event_loop_policy(GLibEventLoopPolicy())
    app = Gtk.Application()
    app.connect("activate", on_activate)
    app.run()
    assert not background_tasks
    print("Bye.")


if __name__ == "__main__":
    main()
