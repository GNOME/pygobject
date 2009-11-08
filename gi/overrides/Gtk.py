import sys

from ..module import DynamicModule

class GtkModule(DynamicModule):

    def __init__(self):
        super(GtkModule, self).__init__()

        initialized, argv = self.init_check(tuple(sys.argv))
        if not initialized:
            raise RuntimeError("Gtk couldn't be initialized")

