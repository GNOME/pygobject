import unittest

from common import gtk

class RadioTest(unittest.TestCase):
    widget_type = None
    constructor_args = ()
    
    def new(self):
        return self.widget_type(*self.constructor_args)

    def new_with_label(self, label):
        return self.widget_type(None, label)

    def new_with_group(self, group):
        return self.widget_type(group)

    def get_label(self, obj):
        return obj.get_property('label')
    
    def compareGroups(self, group1, group2):
        return self.assertEqual(group1, group2)
    
    def testCreate(self):
        if self.widget_type is None:
            return
        radio = self.new()
        self.assert_(isinstance(radio, self.widget_type))

    def testLabel(self):
        if self.widget_type is None:
            return
        radio = self.new_with_label('test-radio')
        self.assertEqual(self.get_label(radio), 'test-radio')

    def testGroup(self):
        if self.widget_type is None:
            return
        radio = self.new()
        radio2 = self.new_with_group(radio)
        self.compareGroups(radio.get_group(), radio2.get_group())
        self.compareGroups(radio2.get_group(), radio.get_group())
        
    def testEmptyGroup(self):
        if self.widget_type is None:
            return
        radio = self.new()
        radio2 = self.new()
        self.compareGroups(radio.get_group(), [radio])
        self.compareGroups(radio2.get_group(), [radio2])
        radio2.set_group(radio)
        self.compareGroups(radio.get_group(), radio2.get_group())
        self.compareGroups(radio2.get_group(), radio.get_group())
        radio2.set_group(None)
        self.compareGroups(radio.get_group(), [radio])
        self.compareGroups(radio2.get_group(), [radio2])

class RadioButtonTest(RadioTest):
    widget_type = gtk.RadioButton
    
class RadioActionTest(RadioTest):
    widget_type = gtk.RadioAction
    constructor_args = ('RadioAction', 'test-radio-action', '', '', 0)
    
    def new_with_group(self, radio):
        obj = self.new()
        obj.set_group(radio)
        return obj

    def new_with_label(self, label):
        return gtk.RadioAction('RadioAction', label, '', '', 0)

class RadioToolButtonTest(RadioTest):
    widget_type = gtk.RadioToolButton
    
    def compareGroups(self, group1, group2):
        return cmp(map(id, group1),
                   map(id, group2))

    def new_with_label(self, label):
        radio = gtk.RadioToolButton(None)
        radio.set_label(label)
        return radio
    
class RadioMenuItem(RadioTest):
    widget_type = gtk.RadioMenuItem

    def get_label(self, obj):
        return obj.get_children()[0].get_text()

if __name__ == '__main__':
    unittest.main()
