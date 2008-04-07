# -*- Mode: Python -*-

import os
import unittest

from common import gio, gobject


class TestFile(unittest.TestCase):
    def setUp(self):
        self._f = open("file.txt", "w+")
        self.file = gio.file_new_for_path("file.txt")

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

        self.file.read_async(0, None, callback)

        loop = gobject.MainLoop()
        loop.run()

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


class TestGFileEnumerator(unittest.TestCase):
    def setUp(self):
        self.file = gio.file_new_for_path(".")

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

        self.stream.read_async(7, 0, None, callback)

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

        self.stream.read_async(10240, 0, None, callback)
        self.stream.read_async(10240, 0, None, callback)

        loop = gobject.MainLoop()
        loop.run()

        self.assertEquals(self.count, 2)

    def testCloseAsync(self):
        def callback(stream, result):
            try:
                self.failUnless(stream.close_finish(result))
            finally:
                loop.quit()

        self.stream.close_async(0, None, callback)

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

        self.stream.write_async("testing", 0, None, callback)

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
        self.stream.write_async("testing", 0, None, callback)

        loop = gobject.MainLoop()
        loop.run()


    def testCloseAsync(self):
        def callback(stream, result):
            try:
                self.failUnless(stream.close_finish(result))
            finally:
                loop.quit()

        self.stream.close_async(0, None, callback)

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
