import unittest

from gi.repository import GLib


class TestUris(unittest.TestCase):
    def testExtractUris(self):
        uri_list_text = """# urn:isbn:0-201-08372-8
http://www.huh.org/books/foo.html
http://www.huh.org/books/foo.pdf
ftp://ftp.foo.org/books/foo.txt
"""
        uri_list = GLib.uri_list_extract_uris(uri_list_text)
        assert uri_list[0] == "http://www.huh.org/books/foo.html"
        assert uri_list[1] == "http://www.huh.org/books/foo.pdf"
        assert uri_list[2] == "ftp://ftp.foo.org/books/foo.txt"
