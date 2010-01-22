from ..importer import modules

Gtk = modules['Gtk']


__all__ = []


import sys

initialized, argv = Gtk.init_check(sys.argv)
if not initialized:
    raise RuntimeError("Gtk couldn't be initialized")
