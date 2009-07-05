import sys

from girepository.module import DynamicModule

class GtkModule(DynamicModule):
    def created(self):
        initialized, argv = self.init_check(tuple(sys.argv))
        if not initialized:
            raise RuntimeError("Gtk couldn't be initialized")

