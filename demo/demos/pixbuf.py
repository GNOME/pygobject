#!/usr/bin/env python
# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Red Hat, Inc., John (J5) Palmieri <johnp@redhat.com>
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

title = "Pixbufs"
description = """A GdkPixbuf represents an image, normally in RGB or RGBA
format. Pixbufs are normally used to load files from disk and perform image
scaling. It also shows off how to use GtkDrawingArea to do a simple animation.
Look at the Image demo for additional pixbuf usage examples.
"""

from gi.repository import Gtk, Gdk, GdkPixbuf, GLib
import os
import math


class PixbufApp:
    FRAME_DELAY = 50
    BACKGROUND_NAME = "background.jpg"
    CYCLE_LEN = 60

    def __init__(self):
        self.background_width = 0
        self.background_height = 0
        self.background_pixbuf = None
        self.frame = None
        self.frame_num = 0
        self.pixbufs = []
        self.image_names = [
            "apple-red.png",
            "gnome-applets.png",
            "gnome-calendar.png",
            "gnome-foot.png",
            "gnome-gmush.png",
            "gnome-gimp.png",
            "gnome-gsame.png",
            "gnu-keys.png"
        ]

        self.window = Gtk.Window(title="Pixbufs")
        self.window.set_resizable(False)
        self.window.connect('destroy', self.cleanup_cb)

        try:
            self.load_pixbufs()
        except GLib.Error as e:
            dialog = Gtk.MessageDialog(None,
                                       0,
                                       Gtk.MessageType.ERROR,
                                       Gtk.ButtonsType.CLOSE,
                                       e.message)

            dialog.run()
            dialog.destroy()
            Gtk.main_quit()

        self.window.set_size_request(self.background_width,
                                     self.background_height)
        self.frame = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB,
                                          False,
                                          8,
                                          self.background_width,
                                          self.background_height)
        self.da = Gtk.DrawingArea()
        self.da.connect('draw', self.draw_cb)
        self.window.add(self.da)
        self.timeout_id = GLib.timeout_add(self.FRAME_DELAY, self.timeout_cb)
        self.window.show_all()

    def load_pixbufs(self):
        base_path = os.path.abspath(os.path.dirname(__file__))
        base_path = os.path.join(base_path, 'data')
        img_path = os.path.join(base_path, self.BACKGROUND_NAME)
        self.background_pixbuf = GdkPixbuf.Pixbuf.new_from_file(img_path)
        self.background_height = self.background_pixbuf.get_height()
        self.background_width = self.background_pixbuf.get_width()

        for img_name in self.image_names:
            img_path = os.path.join(base_path, img_name)
            self.pixbufs.append(GdkPixbuf.Pixbuf.new_from_file(img_path))

    def draw_cb(self, da, cairo_ctx):
        Gdk.cairo_set_source_pixbuf(cairo_ctx, self.frame, 0, 0)
        cairo_ctx.paint()

        return True

    def timeout_cb(self):
        self.background_pixbuf.copy_area(0, 0,
                                         self.background_width,
                                         self.background_height,
                                         self.frame,
                                         0, 0)

        f = float(self.frame_num % self.CYCLE_LEN) / self.CYCLE_LEN

        xmid = self.background_width / 2.0
        ymid = self.background_height / 2.0
        radius = min(xmid, ymid) / 2.0

        r1 = Gdk.Rectangle()
        r2 = Gdk.Rectangle()

        i = 0
        for pb in self.pixbufs:
            i += 1
            ang = 2.0 * math.pi * i / len(self.pixbufs) - f * 2.0 * math.pi

            iw = pb.get_width()
            ih = pb.get_height()

            r = radius + (radius / 3.0) * math.sin(f * 2.0 * math.pi)

            xpos = math.floor(xmid + r * math.cos(ang) - iw / 2.0 + 0.5)
            ypos = math.floor(ymid + r * math.sin(ang) - ih / 2.0 + 0.5)

            if i & 1:
                k = math.sin(f * 2.0 * math.pi)
            else:
                k = math.cos(f * 2.0 * math.pi)

            k = 2.0 * k * k
            k = max(0.25, k)

            r1.x = xpos
            r1.y = ypos
            r1.width = iw * k
            r1.height = ih * k

            r2.x = 0
            r2.y = 0
            r2.width = self.background_width
            r2.height = self.background_height

            success, dest = Gdk.rectangle_intersect(r1, r2)
            if success:
                alpha = 0
                if i & 1:
                    alpha = max(127, math.fabs(255 * math.sin(f * 2.0 * math.pi)))
                else:
                    alpha = max(127, math.fabs(255 * math.cos(f * 2.0 * math.pi)))

                pb.composite(self.frame,
                             dest.x, dest.y,
                             dest.width, dest.height,
                             xpos, ypos,
                             k, k,
                             GdkPixbuf.InterpType.NEAREST,
                             alpha)

        self.da.queue_draw()

        self.frame_num += 1
        return True

    def cleanup_cb(self, widget):
        GLib.source_remove(self.timeout_id)
        Gtk.main_quit()


def main(demoapp=None):
    PixbufApp()
    Gtk.main()

if __name__ == '__main__':
    main()
