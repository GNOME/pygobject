# Copyright 2018 Christoph Reiter <reiter.christoph@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

import pytest

from gi import PyGIDeprecationWarning

GdkPixbuf = pytest.importorskip("gi.repository.GdkPixbuf")


def test_new_from_data():
    width = 600
    height = 32769
    pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, width, height)
    pixels = pixbuf.get_pixels()
    new_pixbuf = GdkPixbuf.Pixbuf.new_from_data(
        pixels,
        GdkPixbuf.Colorspace.RGB,
        True,
        8,
        pixbuf.get_width(),
        pixbuf.get_height(),
        pixbuf.get_rowstride(),
    )
    del pixbuf
    del pixels
    new_pixels = new_pixbuf.get_pixels()
    assert len(new_pixels) == width * height * 4


def test_new_from_data_deprecated_args():
    GdkPixbuf.Pixbuf.new_from_data(b"1234", 0, True, 8, 1, 1, 4)
    GdkPixbuf.Pixbuf.new_from_data(b"1234", 0, True, 8, 1, 1, 4, None)
    with (
        pytest.warns(PyGIDeprecationWarning, match=".*destroy_fn argument.*"),
        pytest.warns(PyGIDeprecationWarning, match=".*destroy_fn_data argument.*"),
    ):
        GdkPixbuf.Pixbuf.new_from_data(
            b"1234", 0, True, 8, 1, 1, 4, object(), object(), object()
        )
