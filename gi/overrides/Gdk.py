from ..types import override
from ..importer import modules

Gdk = modules['Gdk']


class Rectangle(Gdk.Rectangle):

    def __init__(self, x, y, width, height):
        Gdk.Rectangle.__init__(self)
        self.x = x
        self.y = y
        self.width = width
        self.height = height

    def __new__(cls, *args, **kwargs):
        return Gdk.Rectangle.__new__(cls)

    def __repr__(self):
        return '<Gdk.Rectangle(x=%d, y=%d, width=%d, height=%d)>' % (
		    self.x, self.y, self.width, self.height)

Rectangle = override(Rectangle)


__all__ = [Rectangle]


import sys

initialized, argv = Gdk.init_check(sys.argv)
if not initialized:
    raise RuntimeError("Gdk couldn't be initialized")
