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
        assert atk.LAYER_INVALID == 0
        assert atk.LAYER_INVALID != 1
        assert atk.LAYER_INVALID != -1
        assert atk.LAYER_INVALID != atk.LAYER_BACKGROUND

    def testComparisionWarning(self):
        warnings.filterwarnings("error", "", Warning, "", 0)
        try:
            assert atk.LAYER_INVALID != atk.RELATION_NULL
        except Warning:
            pass
        else:
            raise AssertionError
        warnings.resetwarnings()

    def testWindowGetState(self):
        win = gtk.Window()
        win.realize()

        state = win.window.get_state()
        assert state == gdk.WINDOW_STATE_WITHDRAWN
        assert isinstance(state, gdk.WindowState)
        assert 'WINDOW_STATE_WITHDRAWN' in repr(state)

    def testProperty(self):
        win = gtk.Window()

        wtype = win.get_property('type')
        assert wtype == gtk.WINDOW_TOPLEVEL
        assert isinstance(wtype, gtk.WindowType)
        assert 'WINDOW_TOPLEVEL' in repr(wtype)

    def testAtkObj(self):
        obj = atk.NoOpObject(gobject.GObject())
        assert obj.get_role() == atk.ROLE_INVALID

    def testGParam(self):
        win = gtk.Window()
        enums = filter(lambda x: gobject.type_is_a(x.value_type, gobject.GEnum),
                       gobject.list_properties(win))
        assert enums
        enum = enums[0]
        assert hasattr(enum, 'enum_class')
        assert issubclass(enum.enum_class, gobject.GEnum)

    def testWeirdEnumValues(self):
        assert int(gdk.NOTHING) == -1
        assert int(gdk.BUTTON_PRESS) == 4

    def testParamSpec(self):
        props = filter(lambda prop: gobject.type_is_a(prop.value_type, gobject.GEnum),
                       gobject.list_properties(gtk.Window))
        assert len(props)>= 6
        props = filter(lambda prop: prop.name == 'type', props)
        assert props
        prop = props[0]
        klass = prop.enum_class
        assert klass == gtk.WindowType
        assert hasattr(klass, '__enum_values__')
        assert isinstance(klass.__enum_values__, dict)
        assert len(klass.__enum_values__) >= 2

    def testOutofBounds(self):
        val = gtk.icon_size_register('fake', 24, 24)
        assert isinstance(val, gobject.GEnum)
        assert int(val) == 7
        assert '7' in repr(val)
        assert 'GtkIconSize' in repr(val)

class FlagsTest(unittest.TestCase):
    def testFlags(self):
        assert issubclass(gobject.GFlags, int)
        assert isinstance(gdk.BUTTON_PRESS_MASK, gdk.EventMask)
        assert isinstance(gdk.BUTTON_PRESS_MASK, int)
        assert gdk.BUTTON_PRESS_MASK == 256
        assert gdk.BUTTON_PRESS_MASK != 0
        assert gdk.BUTTON_PRESS_MASK != -256
        assert gdk.BUTTON_PRESS_MASK != gdk.BUTTON_RELEASE_MASK

        assert gdk.EventMask.__bases__[0] == gobject.GFlags
        assert len(gdk.EventMask.__flags_values__) == 22

    def testComparisionWarning(self):
        warnings.filterwarnings("error", "", Warning, "", 0)
        try:
            assert gtk.ACCEL_VISIBLE != gtk.EXPAND
        except Warning:
            pass
        else:
            raise AssertionError
        warnings.resetwarnings()
        
    def testFlagOperations(self):
        a = gdk.BUTTON_PRESS_MASK
        assert isinstance(a, gobject.GFlags)
        assert a.first_value_name == 'GDK_BUTTON_PRESS_MASK'
        assert a.first_value_nick == 'button-press-mask'
        assert a.value_names == ['GDK_BUTTON_PRESS_MASK'], a.value_names
        assert a.value_nicks == ['button-press-mask'], a.value_names
        b = gdk.BUTTON_PRESS_MASK | gdk.BUTTON_RELEASE_MASK
        assert isinstance(b, gobject.GFlags)
        assert b.first_value_name == 'GDK_BUTTON_PRESS_MASK'
        assert b.first_value_nick == 'button-press-mask'
        assert b.value_names == ['GDK_BUTTON_PRESS_MASK', 'GDK_BUTTON_RELEASE_MASK']
        assert b.value_nicks == ['button-press-mask', 'button-release-mask']
        c = gdk.BUTTON_PRESS_MASK | gdk.BUTTON_RELEASE_MASK | gdk.ENTER_NOTIFY_MASK
        assert isinstance(c, gobject.GFlags)
        assert c.first_value_name == 'GDK_BUTTON_PRESS_MASK'
        assert c.first_value_nick == 'button-press-mask'
        assert c.value_names == ['GDK_BUTTON_PRESS_MASK', 'GDK_BUTTON_RELEASE_MASK',
                                 'GDK_ENTER_NOTIFY_MASK']
        assert c.value_nicks == ['button-press-mask', 'button-release-mask',
                                 'enter-notify-mask']
        assert int(a)
        assert int(a) == int(gdk.BUTTON_PRESS_MASK)
        assert int(b)
        assert int(b) == (int(gdk.BUTTON_PRESS_MASK) |
                          int(gdk.BUTTON_RELEASE_MASK))
        assert int(c)
        assert int(c) == (int(gdk.BUTTON_PRESS_MASK) |
                          int(gdk.BUTTON_RELEASE_MASK) |
                          int(gdk.ENTER_NOTIFY_MASK))

    def testUnsupportedOpertionWarning(self):
        warnings.filterwarnings("error", "", Warning, "", 0)
        try:
            value = gdk.BUTTON_PRESS_MASK + gdk.BUTTON_RELEASE_MASK
        except Warning:
            pass
        else:
            raise AssertionError
        warnings.resetwarnings()

    def testParamSpec(self):
        props = filter(lambda x: gobject.type_is_a(x.value_type, gobject.GFlags),
                       gtk.container_class_list_child_properties(gtk.Table))
        assert len(props) >= 2
        klass = props[0].flags_class 
        assert klass == gtk.AttachOptions
        assert hasattr(klass, '__flags_values__')
        assert isinstance(klass.__flags_values__, dict)
        assert len(klass.__flags_values__) >= 3

    def testEnumComparision(self):
        enum = gtk.TREE_VIEW_DROP_BEFORE
        assert enum == 0
        assert not enum == 10
        assert not enum != 0
        assert enum != 10
        assert not enum < 0
        assert enum < 10
        assert not enum > 0
        assert not enum > 10
        assert enum >= 0
        assert not enum >= 10
        assert enum <= 0
        assert enum <= 10
        
    def testFlagComparision(self):
        flag = gdk.EXPOSURE_MASK
        assert flag == 2
        assert not flag == 10
        assert not flag != 2
        assert flag != 10
        assert not flag < 2
        assert flag < 10
        assert not flag > 2
        assert not flag > 10
        assert flag >= 2
        assert not flag >= 10
        assert flag <= 2
        assert flag <= 10

if __name__ == '__main__':
    unittest.main()
