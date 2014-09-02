title = "Test Demo"
description = "Dude this is a test"


from gi.repository import Gtk


def _quit(*args):
    Gtk.main_quit()


def main(demoapp=None):
    window = Gtk.Window()
    window.connect_after('destroy', _quit)
    window.show_all()
    Gtk.main()
