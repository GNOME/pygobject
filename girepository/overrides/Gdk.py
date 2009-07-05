import sys

from girepository.module import DynamicModule

class GdkModule(DynamicModule):
    def created(self):
        initialized, argv = self.init_check(tuple(sys.argv))
        if not initialized:
            raise RuntimeError("Gdk couldn't be initialized")

    def rectangle_new(self, x, y, width, height):
        rectangle = self.Rectangle()
        rectangle.x = x
        rectangle.y = y
        rectangle.width = width
        rectangle.height = height
        return rectangle

