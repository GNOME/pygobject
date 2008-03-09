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

class TestGFileEnumerator(unittest.TestCase):
    def setUp(self):
        self.file = gio.file_new_for_path(".")

    def testEnumerateChildren(self):
        found = []

        e = self.file.enumerate_children("standard::*", gio.FILE_QUERY_INFO_NOFOLLOW_SYMLINKS)
        while True:
            info = e.next_file()
            if not info:
                break
            found.append(info.get_name())
        e.close()

        assert 'test_gio.py' in found, found

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
            try:
                read = stream.read_finish(result)
                self.assertEquals(read, len("testing"))
                self.assertEquals(result.get_buffer(), "testing")
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
                #self.assertEquals(result.get_buffer(), None)
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
