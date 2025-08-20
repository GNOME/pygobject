from gi.repository import Gio, GObject


class Person(GObject.Object):
    __gtype_name__ = "Person"

    name = GObject.Property(type=str)

    def __init__(self, name):
        super().__init__()

        self.name = name


class PersonsModel(GObject.GObject, Gio.ListModel):
    __gtype_name__ = "PersonsModel"

    def __init__(self):
        super().__init__()

        # Private list to store the persons
        self._persons = []

    """
    Interface Methods
    """

    def do_get_item(self, position):
        return self._persons[position]

    def do_get_item_type(self):
        return Person

    def do_get_n_items(self):
        return len(self._persons)

    """
    Our Helper Methods
    """

    def add(self, person):
        self._persons.append(person)

        """
        We must call Gio.ListModel.items_changed() every time we change the list.

        It's a helper to emit the "items-changed" signal, so consumer can know
        that the list changed at some point. We pass the position of the change,
        the number of removed items and the number of added items.
        """
        self.items_changed(len(self._persons) - 1, 0, 1)

    def remove(self, position):
        del self._persons[position]
        self.items_changed(position, 1, 0)

    def get_index_by_name(self, name):
        for i, person in enumerate(self._persons):
            if person.name == name:
                return i

        return None
