import sys

from girepository.module import DynamicModule

class GtkModule(DynamicModule):

    def __init__(self, *args):
        super(GtkModule, self).__init__(*args)

        initialized, argv = self.init_check(tuple(sys.argv))
        if not initialized:
            raise RuntimeError("Gtk couldn't be initialized")

