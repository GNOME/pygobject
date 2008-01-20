# -*- Mode: Python -*-

import os
import unittest

from common import gio, gobject


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
            read = stream.read_finish(result)
            self.assertEquals(read, len("testing"))
            self.assertEquals(result.get_buffer(), "testing")
            stream.close()
            loop.quit()

        self.stream.read_async(7, 0, None, callback)

        loop = gobject.MainLoop()
        loop.run()

    def testReadAsyncError(self):
        self.count = 0
        def callback(stream, result):
            #self.assertEquals(result.get_buffer(), None)
            self.count += 1
            if self.count == 1:
                return
            self.assertRaises(gobject.GError, stream.read_finish, result)
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

