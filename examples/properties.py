from gi.repository import GObject


class MyObject(GObject.GObject):

    foo = GObject.Property(type=str, default='bar')
    boolprop = GObject.Property(type=bool, default=False)

    def __init__(self):
        GObject.GObject.__init__(self)

    @GObject.Property
    def readonly(self):
        return 'readonly'

GObject.type_register(MyObject)

print("MyObject properties: ", list(MyObject.props))

obj = MyObject()

print("obj.foo ==", obj.foo)

obj.foo = 'spam'
print("obj.foo = spam")

print("obj.foo == ", obj.foo)

print("obj.boolprop == ", obj.boolprop)

print(obj.readonly)
obj.readonly = 'does-not-work'
