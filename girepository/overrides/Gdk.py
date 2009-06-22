from girepository.module import DynamicModule

class GdkModule(DynamicModule):
    def created(self):
        self.init_check(0, None)

    def rectangle_new(self, x, y, width, height):
        rectangle = self.Rectangle()
        rectangle.x = x
        rectangle.y = y
        rectangle.width = width
        rectangle.height = height
        return rectangle

