# -*- Mode: Python -*-

import os
import unittest

from common import gio, gobject


class TestFile(unittest.TestCase):
    def setUp(self):
        self._f = open("file.txt", "w+")
        self.file = gio.File("file.txt")

    def tearDown(self):
        self._f.close()
        os.unlink("file.txt")

    def testReadAsync(self):
        self._f.write("testing")
        self._f.seek(0)

        def callback(file, result):
            try:
                stream = file.read_finish(result)
                self.failUnless(isinstance(stream, gio.InputStream))
                self.assertEquals(stream.read(), "testing")
            finally:
                loop.quit()

        self.file.read_async(callback)

        loop = gobject.MainLoop()
        loop.run()

    def testReadAsyncError(self):
        self.assertRaises(TypeError, self.file.read_async)
        self.assertRaises(TypeError, self.file.read_async, "foo", "bar")
        self.assertRaises(TypeError, self.file.read_async, "foo",
                          priority="bar")
        self.assertRaises(TypeError, self.file.read_async, "foo",
                          priority="bar")
        self.assertRaises(TypeError, self.file.read_async, "foo",
                          priority=1, cancellable="bar")
        self.assertRaises(TypeError, self.file.read_async, "foo", 1, "bar")

    def testConstructor(self):
        for gfile in [gio.File("/"),
                      gio.File("file:///"),
                      gio.File(uri="file:///"),
                      gio.File(path="/"),
                      gio.File(u"/"),
                      gio.File(path=u"/")]:
            self.failUnless(isinstance(gfile, gio.File))
            self.assertEquals(gfile.get_path(), "/")
            self.assertEquals(gfile.get_uri(), "file:///")

    def testConstructorError(self):
        self.assertRaises(TypeError, gio.File)
        self.assertRaises(TypeError, gio.File, 1)
        self.assertRaises(TypeError, gio.File, "foo", "bar")
        self.assertRaises(TypeError, gio.File, foo="bar")
        self.assertRaises(TypeError, gio.File, uri=1)
        self.assertRaises(TypeError, gio.File, path=1)
        
    def testLoadContents(self):
        self._f.write("testing load_contents")
        self._f.seek(0)
        c = gio.Cancellable()
        cont, leng, etag = self.file.load_contents(cancellable=c)
        self.assertEqual(cont, "testing load_contents")
        self.assertEqual(leng, 21)
        self.assertNotEqual(etag, '')
    
    def testLoadContentsAsync(self):
        self._f.write("testing load_contents_async")
        self._f.seek(0)
        
        def callback(contents, result):
            try:
                cont, leng, etag = contents.load_contents_finish(result)
                self.assertEqual(cont, "testing load_contents_async")
                self.assertEqual(leng, 27)
                self.assertNotEqual(etag, '')
            finally:
                loop.quit()
        
        canc = gio.Cancellable()
        self.file.load_contents_async(callback, cancellable=canc)
        
        loop = gobject.MainLoop()
        loop.run()

class TestGFileEnumerator(unittest.TestCase):
    def setUp(self):
        self.file = gio.File(".")

    def testEnumerateChildren(self):
        enumerator = self.file.enumerate_children(
            "standard::*", gio.FILE_QUERY_INFO_NOFOLLOW_SYMLINKS)
        for file_info in enumerator:
            if file_info.get_name() == 'test_gio.py':
                break
        else:
            raise AssertionError


class TestInputStream(unittest.TestCase):
    def setUp(self):
        self._f = open("inputstream.txt", "w+")
        self._f.write("testing")
        self._f.seek(0)
        self.stream = gio.unix.InputStream(self._f.fileno(), False)

    def tearDown(self):
        self._f.close()
        os.unlink("inputstream.txt")

    def testRead(self):
        self.assertEquals(self.stream.read(), "testing")

    def testReadAsync(self):
        def callback(stream, result):
            self.assertEquals(result.get_op_res_gssize(), 7)
            try:
                data = stream.read_finish(result)
                self.assertEquals(data, "testing")
                stream.close()
            finally:
                loop.quit()

        self.stream.read_async(7, callback)

        loop = gobject.MainLoop()
        loop.run()

    def testReadAsyncError(self):
        self.count = 0
        def callback(stream, result):
            try:
                self.count += 1
                if self.count == 1:
                    return
                self.assertRaises(gobject.GError, stream.read_finish, result)
            finally:
                loop.quit()

        self.stream.read_async(10240, callback)
        self.stream.read_async(10240, callback)

        loop = gobject.MainLoop()
        loop.run()

        self.assertEquals(self.count, 2)

        self.assertRaises(TypeError, self.stream.read_async)
        self.assertRaises(TypeError, self.stream.read_async, "foo")
        self.assertRaises(TypeError, self.stream.read_async, 1024, "bar")
        self.assertRaises(TypeError, self.stream.read_async, 1024,
                          priority="bar")
        self.assertRaises(TypeError, self.stream.read_async, 1024,
                          priority="bar")
        self.assertRaises(TypeError, self.stream.read_async, 1024,
                          priority=1, cancellable="bar")
        self.assertRaises(TypeError, self.stream.read_async, 1024, 1, "bar")


    # FIXME: this makes 'make check' freeze
    def _testCloseAsync(self):
        def callback(stream, result):
            try:
                self.failUnless(stream.close_finish(result))
            finally:
                loop.quit()

        self.stream.close_async(callback)

        loop = gobject.MainLoop()
        loop.run()


class TestOutputStream(unittest.TestCase):
    def setUp(self):
        self._f = open("outputstream.txt", "w")
        self.stream = gio.unix.OutputStream(self._f.fileno(), False)

    def tearDown(self):
        self._f.close()
        os.unlink("outputstream.txt")

    def testWrite(self):
        self.stream.write("testing")
        self.stream.close()
        self.failUnless(os.path.exists("outputstream.txt"))
        self.assertEquals(open("outputstream.txt").read(), "testing")

    def testWriteAsync(self):
        def callback(stream, result):
            self.assertEquals(result.get_op_res_gssize(), 7)
            try:
                self.assertEquals(stream.write_finish(result), 7)
                self.failUnless(os.path.exists("outputstream.txt"))
                self.assertEquals(open("outputstream.txt").read(), "testing")
            finally:
                loop.quit()

        self.stream.write_async("testing", callback)

        loop = gobject.MainLoop()
        loop.run()

    def testWriteAsyncError(self):
        def callback(stream, result):
            self.assertEquals(result.get_op_res_gssize(), 0)
            try:
                self.assertRaises(gobject.GError, stream.write_finish, result)
            finally:
                loop.quit()

        self.stream.close()
        self.stream.write_async("testing", callback)

        loop = gobject.MainLoop()
        loop.run()

        self.assertRaises(TypeError, self.stream.write_async)
        self.assertRaises(TypeError, self.stream.write_async, 1138)
        self.assertRaises(TypeError, self.stream.write_async, "foo", "bar")
        self.assertRaises(TypeError, self.stream.write_async, "foo",
                          priority="bar")
        self.assertRaises(TypeError, self.stream.write_async, "foo",
                          priority="bar")
        self.assertRaises(TypeError, self.stream.write_async, "foo",
                          priority=1, cancellable="bar")
        self.assertRaises(TypeError, self.stream.write_async, "foo", 1, "bar")

    # FIXME: this makes 'make check' freeze
    def _testCloseAsync(self):
        def callback(stream, result):
            try:
                self.failUnless(stream.close_finish(result))
            finally:
                loop.quit()

        self.stream.close_async(callback)

        loop = gobject.MainLoop()
        loop.run()


class TestVolumeMonitor(unittest.TestCase):
    def setUp(self):
        self.monitor = gio.volume_monitor_get()

    def testGetConnectedDrives(self):
        drives = self.monitor.get_connected_drives()
        self.failUnless(isinstance(drives, list))

    def testGetVolumes(self):
        volumes = self.monitor.get_volumes()
        self.failUnless(isinstance(volumes, list))

    def testGetMounts(self):
        mounts = self.monitor.get_mounts()
        self.failUnless(isinstance(mounts, list))
        if not mounts:
            return
        
        self.failUnless(isinstance(mounts[0], gio.Mount))
        # Bug 538601
        icon = mounts[0].get_icon()
        if not icon:
            return
        self.failUnless(isinstance(icon, gio.Icon))
    

class TestThemedIcon(unittest.TestCase):
    def setUp(self):
        self.icon = gio.ThemedIcon("open")

    def testGetNames(self):
        self.assertEquals(self.icon.get_names(), ['open'])

    def testAppendName(self):
        self.assertEquals(self.icon.get_names(), ['open'])
        self.icon.append_name('close')
        self.assertEquals(self.icon.get_names(), ['open', 'close'])


class TestContentTypeGuess(unittest.TestCase):
    def testFromName(self):
        mime_type = gio.content_type_guess('diagram.svg')
        self.assertEquals('image/svg+xml', mime_type)

    def testFromContents(self):
        mime_type = gio.content_type_guess(data='<html></html>')
        self.assertEquals('text/html', mime_type)

    def testFromContentsUncertain(self):
        mime_type, result_uncertain = gio.content_type_guess(
            data='<html></html>', want_uncertain=True)
        self.assertEquals('text/html', mime_type)
        self.assertEquals(bool, type(result_uncertain))


class TestFileInfo(unittest.TestCase):
    def testListAttributes(self):
        fileinfo = gio.File("test_gio.py").query_info("*")
        attributes = fileinfo.list_attributes("standard")
        self.failUnless(attributes)
        self.failUnless('standard::name' in attributes)


class TestAppInfo(unittest.TestCase):
    def setUp(self):
        self.appinfo = gio.AppInfo("does-not-exist")

    def testSimple(self):
        self.assertEquals(self.appinfo.get_description(),
                          "Custom definition for does-not-exist")
