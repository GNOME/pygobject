from gi.repository import GObject


class PyGObject(GObject.GObject):
    __gtype_name__ = 'PyGObject'
    __gproperties__ = {
        'label': (GObject.TYPE_STRING,
                  'label property',
                  'the label of the object',
                  'default', GObject.PARAM_READWRITE),
        }

    def __init__(self):
        self._props = {}
        GObject.GObject.__init__(self)
        self.set_property('label', 'hello')

    def do_set_property(self, name, value):
        self._props[name] = value

    def do_get_property(self, name):
        return self._props[name]
