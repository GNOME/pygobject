import asyncio

from gi.events import GLibEventLoopPolicy
from gi.repository import Gtk

background_tasks = set()

def create_background_task(coro):
    task = asyncio.create_task(coro)
    background_tasks.add(task)
    task.add_done_callback(background_tasks.discard)
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

    create_background_task(update_label(label))

    win.present()

def main():
    asyncio.set_event_loop_policy(GLibEventLoopPolicy())
    app = Gtk.Application()
    app.connect("activate", on_activate)
    app.run()

if __name__ == "__main__":
    main()
