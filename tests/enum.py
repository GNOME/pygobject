import unittest
import warnings

from common import gobject, atk, pango, gtk, gdk

class EnumTest(unittest.TestCase):
    def testEnums(self):
        assert issubclass(gobject.GEnum, int)
        assert isinstance(atk.LAYER_OVERLAY, atk.Layer)
        assert isinstance(atk.LAYER_OVERLAY, int)
        assert 'LAYER_OVERLAY' in repr(atk.LAYER_OVERLAY)
        assert int(atk.LAYER_OVERLAY)
        warnings.filterwarnings("error", "", Warning, "", 0)
        try:
            assert atk.LAYER_INVALID != atk.RELATION_NULL
        except Warning:
            pass
        else:
            raise AssertionError
        warnings.resetwarnings()
        assert atk.LAYER_INVALID == 0
        assert atk.LAYER_INVALID != 1
        assert atk.LAYER_INVALID != -1
        assert atk.LAYER_INVALID != atk.LAYER_BACKGROUND

    def testWindowGetState(self):
        win = gtk.Window()
        win.realize()

        state = win.window.get_state()
        assert state == gdk.WINDOW_STATE_ICONIFIED
        assert isinstance(state, gdk.WindowState)
        assert 'WINDOW_STATE_ICONIFIED' in repr(state)

    def testProperty(self):
        win = gtk.Window()

        wtype = win.get_property('type')
        assert wtype == gtk.WINDOW_TOPLEVEL
        assert isinstance(wtype, gtk.WindowType)
        assert 'WINDOW_TOPLEVEL' in repr(wtype)

    def testAtkObj(self):
        obj = atk.NoOpObject(gobject.GObject())
        assert obj.get_role() == atk.ROLE_INVALID

class FlagsTest(unittest.TestCase):
    def testFlags(self):
        assert issubclass(gobject.GFlags, int)
        assert isinstance(gdk.BUTTON_PRESS_MASK, gdk.EventMask)
        assert isinstance(gdk.BUTTON_PRESS_MASK, int)
        warnings.filterwarnings("error", "", Warning, "", 0)
        try:
            assert gtk.ACCEL_VISIBLE != gtk.EXPAND
        except Warning:
            pass
        else:
            raise AssertionError
        warnings.resetwarnings()
        assert gdk.BUTTON_PRESS_MASK == 256
        assert gdk.BUTTON_PRESS_MASK != 0
        assert gdk.BUTTON_PRESS_MASK != -256
        assert gdk.BUTTON_PRESS_MASK != gdk.BUTTON_RELEASE_MASK

        assert gdk.EventMask.__bases__[0] == gobject.GFlags
        assert len(gdk.EventMask.__flags_values__) == 22
        a = gdk.BUTTON_PRESS_MASK
        b = gdk.BUTTON_PRESS_MASK | gdk.BUTTON_RELEASE_MASK
        c = gdk.BUTTON_PRESS_MASK | gdk.BUTTON_RELEASE_MASK | gdk.ENTER_NOTIFY_MASK
        assert int(a)
        assert int(a) == int(gdk.BUTTON_PRESS_MASK)
        assert int(b)
        assert int(b) == (int(gdk.BUTTON_PRESS_MASK) |
                          int(gdk.BUTTON_RELEASE_MASK))
        assert int(c)
        assert int(c) == (int(gdk.BUTTON_PRESS_MASK) |
                          int(gdk.BUTTON_RELEASE_MASK) |
                          int(gdk.ENTER_NOTIFY_MASK))

        warnings.filterwarnings("error", "", Warning, "", 0)
        try:
            value = gdk.BUTTON_PRESS_MASK + gdk.BUTTON_RELEASE_MASK
        except Warning:
            pass
        else:
            raise AssertionError
        warnings.resetwarnings()

if __name__ == '__main__':
    unittest.main()
