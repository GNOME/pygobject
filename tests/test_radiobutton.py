import unittest

from common import gtk

class RadioButtonTest(unittest.TestCase):
    def testCreate(self):
        radio = gtk.RadioButton()
        self.assert_(isinstance(radio, gtk.RadioButton))

    def testLabel(self):
        radio = gtk.RadioButton(None, 'test-radio')
        self.assertEqual(radio.get_label(), 'test-radio')

    def testGroup(self):
        radio = gtk.RadioButton()
        radio2 = gtk.RadioButton(radio)
        self.assertEqual(radio.get_group(), radio2.get_group())
        
    def testEmptyGroup(self):
        radio = gtk.RadioButton()
        radio2 = gtk.RadioButton()
        self.assertEqual(radio.get_group(), [radio])
        self.assertEqual(radio2.get_group(), [radio2])
        radio2.set_group(radio)
        self.assertEqual(radio.get_group(), radio2.get_group())
        radio2.set_group(None)
        self.assertEqual(radio.get_group(), [radio])
        self.assertEqual(radio2.get_group(), [radio2])

if __name__ == '__main__':
    unittest.main()
