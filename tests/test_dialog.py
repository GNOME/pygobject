import unittest

from common import gtk

class DialogTest(unittest.TestCase):
    def testDialogAdd(self):
        dialog = gtk.MessageDialog()
        
        # sys.maxint + 1
        response_id = 2147483648
        self.assertRaises(OverflowError, dialog.add_button, "Foo", response_id)
        self.assertRaises(OverflowError, dialog.add_buttons, "Foo", response_id)

if __name__ == '__main__':
    unittest.main()
