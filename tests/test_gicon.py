# -*- Mode: Python -*-

import os
import unittest

from common import gio, glib


class TestIcon(unittest.TestCase):
    def test_eq(self):
        self.assertEquals(gio.File('foo.png').icon_new(),
                          gio.File('foo.png').icon_new())
        self.assertEquals(gio.ThemedIcon('foo'),
                          gio.ThemedIcon('foo'))

        self.assertNotEqual(gio.File('foo.png').icon_new(),
                            gio.File('bar.png').icon_new())
        self.assertNotEquals(gio.ThemedIcon('foo'),
                             gio.ThemedIcon('bar'))
        self.assertNotEquals(gio.File('foo.png').icon_new(),
                             gio.ThemedIcon('foo'))

    def test_hash(self):
        self.assertEquals(hash(gio.File('foo.png').icon_new()),
                          hash(gio.File('foo.png').icon_new()))
        self.assertEquals(hash(gio.ThemedIcon('foo')),
                          hash(gio.ThemedIcon('foo')))

class TestLoadableIcon(unittest.TestCase):
    def setUp(self):
        self.file = open('temp.svg', 'w')
        self.svg = ('<?xml version="1.0" encoding="UTF-8" standalone="no"?>'
                    '<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.0//EN" '
                    '"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd">'
                    '<svg width="32" height="32"/>')
        self.file.write(self.svg)
        self.file.close()
        self.icon = gio.File('temp.svg').icon_new()

    def tearDown(self):
        if os.path.exists('temp.svg'):
            os.unlink('temp.svg')

    def test_load(self):
        stream, type = self.icon.load()
        try:
            self.assert_(isinstance(stream, gio.InputStream))
            self.assertEquals(self.svg, stream.read())
        finally:
            stream.close()

    def test_load_async(self):
        def callback(icon, result):
            try:
                stream, type = icon.load_finish(result)
                self.assert_(isinstance(stream, gio.InputStream))
                self.assertEquals(self.svg, stream.read())
            finally:
                loop.quit()
                stream.close()

        self.icon.load_async(callback)

        loop = glib.MainLoop()
        loop.run()

class TestThemedIcon(unittest.TestCase):
    def setUp(self):
        self.icon = gio.ThemedIcon("open")

    def test_constructor(self):
        icon = gio.ThemedIcon('foo')
        self.assertEquals(['foo'], icon.props.names)
        self.assert_(not icon.props.use_default_fallbacks)

        icon = gio.ThemedIcon(['foo', 'bar', 'baz'])
        self.assertEquals(['foo', 'bar', 'baz'], icon.props.names)
        self.assert_(not icon.props.use_default_fallbacks)

        icon = gio.ThemedIcon('xxx-yyy-zzz', True)
        self.assertEquals(['xxx-yyy-zzz', 'xxx-yyy', 'xxx'], icon.props.names)
        self.assert_(icon.props.use_default_fallbacks)

    def test_constructor_illegals(self):
        self.assertRaises(TypeError, lambda: gio.ThemedIcon(42))
        self.assertRaises(TypeError, lambda: gio.ThemedIcon(['foo', 42, 'bar']))

    def test_get_names(self):
        self.assertEquals(self.icon.get_names(), ['open'])

    def test_append_name(self):
        self.assertEquals(self.icon.get_names(), ['open'])
        self.icon.append_name('close')
        self.assertEquals(self.icon.get_names(), ['open', 'close'])
