from girepository.module import DynamicModule

class GtkModule(DynamicModule):
    def created(self):
        self.init_check(0, None)
        #if self.init_check(len(sys.argv), sys.argv):
        #    raise RuntimeError("could not open display")

