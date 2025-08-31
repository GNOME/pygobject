import os
import unittest
import tempfile
import os.path
import shutil
import warnings

try:
    import fcntl
except ImportError:
    fcntl = None

from gi.repository import GLib
from gi import PyGIDeprecationWarning


class IOChannel(unittest.TestCase):
    def setUp(self):
        self.workdir = tempfile.mkdtemp()

        self.testutf8 = os.path.join(self.workdir, "testutf8.txt")
        with open(self.testutf8, "wb") as f:
            f.write(
                """hello ♥ world
second line

À demain!""".encode()
            )

        self.testlatin1 = os.path.join(self.workdir, "testlatin1.txt")
        with open(self.testlatin1, "wb") as f:
            f.write(b"""hell\xf8 world
second line

\xc0 demain!""")

        self.testout = os.path.join(self.workdir, "testout.txt")

    def tearDown(self):
        shutil.rmtree(self.workdir)

    def test_file_readline_utf8(self):
        ch = GLib.IOChannel(filename=self.testutf8)
        self.assertEqual(ch.get_encoding(), "UTF-8")
        self.assertTrue(ch.get_close_on_unref())
        self.assertEqual(ch.readline(), "hello ♥ world\n")
        self.assertEqual(ch.get_buffer_condition(), GLib.IOCondition.IN)
        self.assertEqual(ch.readline(), "second line\n")
        self.assertEqual(ch.readline(), "\n")
        self.assertEqual(ch.readline(), "À demain!")
        self.assertEqual(ch.get_buffer_condition(), 0)
        self.assertEqual(ch.readline(), "")
        ch.shutdown(True)

    def test_file_readline_latin1(self):
        ch = GLib.IOChannel(filename=self.testlatin1, mode="r")
        ch.set_encoding("latin1")
        self.assertEqual(ch.get_encoding(), "latin1")
        self.assertEqual(ch.readline(), "hellø world\n")
        self.assertEqual(ch.readline(), "second line\n")
        self.assertEqual(ch.readline(), "\n")
        self.assertEqual(ch.readline(), "À demain!")
        ch.shutdown(True)

    def test_file_iter(self):
        items = []
        ch = GLib.IOChannel(filename=self.testutf8)
        for item in ch:
            items.append(item)  # noqa: PERF402
        self.assertEqual(len(items), 4)
        self.assertEqual(items[0], "hello ♥ world\n")
        ch.shutdown(True)

    def test_file_readlines(self):
        ch = GLib.IOChannel(filename=self.testutf8)
        lines = ch.readlines()
        # Note, this really ought to be 4, but the static bindings add an extra
        # empty one
        self.assertGreaterEqual(len(lines), 4)
        self.assertLessEqual(len(lines), 5)
        self.assertEqual(lines[0], "hello ♥ world\n")
        self.assertEqual(lines[3], "À demain!")
        if len(lines) == 4:
            self.assertEqual(lines[4], "")

    def test_file_read(self):
        ch = GLib.IOChannel(filename=self.testutf8)
        with open(self.testutf8, "rb") as f:
            self.assertEqual(ch.read(), f.read())

        ch = GLib.IOChannel(filename=self.testutf8)
        with open(self.testutf8, "rb") as f:
            self.assertEqual(ch.read(10), f.read(10))

        ch = GLib.IOChannel(filename=self.testutf8)
        with open(self.testutf8, "rb") as f:
            self.assertEqual(ch.read(max_count=15), f.read(15))

    def test_file_read_chars(self):
        ch = GLib.IOChannel(filename=self.testutf8)
        with open(self.testutf8, "rb") as f:
            self.assertEqual(ch.read_chars(), f.read())

    def test_seek(self):
        ch = GLib.IOChannel(filename=self.testutf8)
        ch.seek(2)
        self.assertEqual(ch.read(3), b"llo")

        ch.seek(2, 0)  # SEEK_SET
        self.assertEqual(ch.read(3), b"llo")

        ch.seek(1, 1)  # SEEK_CUR, skip the space
        self.assertEqual(ch.read(3), b"\xe2\x99\xa5")

        ch.seek(2, 2)  # SEEK_END
        # FIXME: does not work currently
        # self.assertEqual(ch.read(2), b'n!')

        # invalid whence value
        self.assertRaises(ValueError, ch.seek, 0, 3)
        ch.shutdown(True)

    def test_file_write(self):
        ch = GLib.IOChannel(filename=self.testout, mode="w")
        ch.set_encoding("latin1")
        ch.write("hellø world\n")
        ch.shutdown(True)
        ch = GLib.IOChannel(filename=self.testout, mode="a")
        ch.set_encoding("latin1")
        ch.write("À demain!")
        ch.shutdown(True)

        with open(self.testout, "rb") as f:
            self.assertEqual(f.read().decode("latin1"), "hellø world\nÀ demain!")

    def test_file_writelines(self):
        ch = GLib.IOChannel(filename=self.testout, mode="w")
        ch.writelines(["foo", "bar\n", "baz\n", "end"])
        ch.shutdown(True)

        with open(self.testout) as f:
            self.assertEqual(f.read(), "foobar\nbaz\nend")

    def test_buffering(self):
        writer = GLib.IOChannel(filename=self.testout, mode="w")
        writer.set_encoding(None)
        self.assertTrue(writer.get_buffered())
        self.assertGreater(writer.get_buffer_size(), 10)

        reader = GLib.IOChannel(filename=self.testout, mode="r")

        # does not get written immediately on buffering
        writer.write("abc")
        self.assertEqual(reader.read(), b"")
        writer.flush()
        self.assertEqual(reader.read(), b"abc")

        # does get written immediately without buffering
        writer.set_buffered(False)
        writer.write("def")
        self.assertEqual(reader.read(), b"def")

        # writes after buffer overflow
        writer.set_buffer_size(10)
        writer.write("0123456789012")
        self.assertTrue(reader.read().startswith(b"012"))
        writer.flush()
        reader.read()  # ignore bits written after flushing

        # closing flushes
        writer.set_buffered(True)
        writer.write("ghi")
        writer.shutdown(True)
        self.assertEqual(reader.read(), b"ghi")
        reader.shutdown(True)

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_fd_read(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)
        self.assertNotEqual(ch.get_flags() | GLib.IOFlags.NONBLOCK, 0)
        self.assertEqual(ch.read(), b"")
        os.write(w, b"\x01\x02")
        self.assertEqual(ch.read(), b"\x01\x02")

        # now test blocking case, after closing the write end
        ch.set_flags(GLib.IOFlags(ch.get_flags() & ~GLib.IOFlags.NONBLOCK))
        os.write(w, b"\x03\x04")
        os.close(w)
        self.assertEqual(ch.read(), b"\x03\x04")

        ch.shutdown(True)

    @unittest.skipUnless(fcntl, "no fcntl")
    def test_fd_write(self):
        (r, w) = os.pipe()
        fcntl.fcntl(r, fcntl.F_SETFL, fcntl.fcntl(r, fcntl.F_GETFL) | os.O_NONBLOCK)

        ch = GLib.IOChannel(filedes=w, mode="w")
        ch.set_encoding(None)
        ch.set_buffered(False)
        ch.write(b"\x01\x02")
        self.assertEqual(os.read(r, 10), b"\x01\x02")

        # now test blocking case, after closing the write end
        fcntl.fcntl(r, fcntl.F_SETFL, fcntl.fcntl(r, fcntl.F_GETFL) & ~os.O_NONBLOCK)
        ch.write(b"\x03\x04")
        ch.shutdown(True)
        self.assertEqual(os.read(r, 10), b"\x03\x04")
        os.close(r)

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_deprecated_method_add_watch_no_data(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        # io_add_watch() method is deprecated, use GLib.io_add_watch
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            ch.add_watch(GLib.IOCondition.IN, cb, priority=GLib.PRIORITY_HIGH)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        ml = GLib.MainLoop()
        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_deprecated_method_add_watch_data_priority(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition, data):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            self.assertEqual(data, "hello")
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        ml = GLib.MainLoop()
        # io_add_watch() method is deprecated, use GLib.io_add_watch
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            id = ch.add_watch(
                GLib.IOCondition.IN, cb, "hello", priority=GLib.PRIORITY_HIGH
            )
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_add_watch_no_data(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        id = GLib.io_add_watch(ch, GLib.PRIORITY_HIGH, GLib.IOCondition.IN, cb)

        ml = GLib.MainLoop()
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_add_watch_with_data(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition, data):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            self.assertEqual(data, "hello")
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        id = GLib.io_add_watch(ch, GLib.PRIORITY_HIGH, GLib.IOCondition.IN, cb, "hello")

        ml = GLib.MainLoop()
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_add_watch_with_multi_data(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition, data1, data2, data3):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            self.assertEqual(data1, "a")
            self.assertEqual(data2, "b")
            self.assertEqual(data3, "c")
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        id = GLib.io_add_watch(
            ch, GLib.PRIORITY_HIGH, GLib.IOCondition.IN, cb, "a", "b", "c"
        )

        ml = GLib.MainLoop()
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_deprecated_add_watch_no_data(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            id = GLib.io_add_watch(
                ch, GLib.IOCondition.IN, cb, priority=GLib.PRIORITY_HIGH
            )
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    @unittest.skipIf(os.name == "nt", "NONBLOCK not implemented on Windows")
    def test_deprecated_add_watch_with_data(self):
        (r, w) = os.pipe()

        ch = GLib.IOChannel(filedes=r)
        ch.set_encoding(None)
        ch.set_flags(ch.get_flags() | GLib.IOFlags.NONBLOCK)

        cb_reads = []

        def cb(channel, condition, data):
            self.assertEqual(channel, ch)
            self.assertEqual(condition, GLib.IOCondition.IN)
            self.assertEqual(data, "hello")
            cb_reads.append(channel.read())
            if len(cb_reads) == 2:
                ml.quit()
            return True

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            id = GLib.io_add_watch(
                ch, GLib.IOCondition.IN, cb, "hello", priority=GLib.PRIORITY_HIGH
            )
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        GLib.idle_add(write)

        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(cb_reads, [b"a", b"b"])

    def test_backwards_compat_flags(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            self.assertEqual(GLib.IOCondition.IN, GLib.IO_IN)
            self.assertEqual(GLib.IOFlags.NONBLOCK, GLib.IO_FLAG_NONBLOCK)
            self.assertEqual(GLib.IOFlags.IS_SEEKABLE, GLib.IO_FLAG_IS_SEEKABLE)
            self.assertEqual(GLib.IOStatus.NORMAL, GLib.IO_STATUS_NORMAL)
