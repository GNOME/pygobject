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

title = "Images"
description = """GtkImage is used to display an image; the image can be in a
number of formats. Typically, you load an image into a GdkPixbuf, then display
the pixbuf. This demo code shows some of the more obscure cases, in the simple
case a call to gtk_image_new_from_file() is all you need.
"""

import os
from os import path

from gi.repository import Gtk, Gdk, GdkPixbuf, GLib, Gio, GObject


class ImagesApp:
    def __init__(self):
        self.pixbuf_loader = None
        self.image_stream = None

        self.window = Gtk.Window(title="Images")
        self.window.connect('destroy', self.cleanup_cb)
        self.window.set_border_width(8)

        vbox = Gtk.VBox(spacing=8)
        vbox.set_border_width(8)
        self.window.add(vbox)

        label = Gtk.Label()
        label.set_markup('<u>Image loaded from file</u>')
        vbox.pack_start(label, False, False, 0)

        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)

        # The alignment keeps the frame from growing when users resize
        # the window
        align = Gtk.Alignment(xalign=0.5,
                              yalign=0.5,
                              xscale=0,
                              yscale=0)
        align.add(frame)
        vbox.pack_start(align, False, False, 0)

        self.base_path = os.path.abspath(os.path.dirname(__file__))
        self.base_path = os.path.join(self.base_path, 'data')
        filename = os.path.join(self.base_path, 'gtk-logo-rgb.gif')
        pixbuf = GdkPixbuf.Pixbuf.new_from_file(filename)
        transparent = pixbuf.add_alpha(True, 0xff, 0xff, 0xff)
        image = Gtk.Image.new_from_pixbuf(transparent)
        frame.add(image)

        # Animation

        label = Gtk.Label()
        label.set_markup('<u>Animation loaded from file</u>')
        vbox.pack_start(label, False, False, 0)

        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)

        # The alignment keeps the frame from growing when users resize
        # the window
        align = Gtk.Alignment(xalign=0.5,
                              yalign=0.5,
                              xscale=0,
                              yscale=0)
        align.add(frame)
        vbox.pack_start(align, False, False, 0)

        img_path = path.join(self.base_path, 'floppybuddy.gif')
        image = Gtk.Image.new_from_file(img_path)
        frame.add(image)

        # Symbolic icon

        label = Gtk.Label()
        label.set_markup('<u>Symbolic themed icon</u>')
        vbox.pack_start(label, False, False, 0)

        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)

        # The alignment keeps the frame from growing when users resize
        # the window
        align = Gtk.Alignment(xalign=0.5,
                              yalign=0.5,
                              xscale=0,
                              yscale=0)
        align.add(frame)
        vbox.pack_start(align, False, False, 0)

        gicon = Gio.ThemedIcon.new_with_default_fallbacks('battery-caution-charging-symbolic')
        image = Gtk.Image.new_from_gicon(gicon, Gtk.IconSize.DIALOG)
        frame.add(image)

        # progressive

        label = Gtk.Label()
        label.set_markup('<u>Progressive image loading</u>')
        vbox.pack_start(label, False, False, 0)

        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)

        # The alignment keeps the frame from growing when users resize
        # the window
        align = Gtk.Alignment(xalign=0.5,
                              yalign=0.5,
                              xscale=0,
                              yscale=0)
        align.add(frame)
        vbox.pack_start(align, False, False, 0)

        image = Gtk.Image.new_from_pixbuf(None)
        frame.add(image)

        self.start_progressive_loading(image)

        # Sensistivity control
        button = Gtk.ToggleButton.new_with_mnemonic('_Insensitive')
        button.connect('toggled', self.toggle_sensitivity_cb, vbox)
        vbox.pack_start(button, False, False, 0)

        self.window.show_all()

    def toggle_sensitivity_cb(self, button, container):
        widget_list = container.get_children()
        for w in widget_list:
            if w != button:
                w.set_sensitive(not button.get_active())

        return True

    def progressive_timeout(self, image):
        # This shows off fully-paranoid error handling, so looks scary.
        # You could factor out the error handling code into a nice separate
        # function to make things nicer.

        if self.image_stream:
            try:
                buf = self.image_stream.read(256)
            except IOError as e:
                dialog = Gtk.MessageDialog(self.window,
                                           Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                           Gtk.MessageType.ERROR,
                                           Gtk.ButtonsType.CLOSE,
                                           "Failure reading image file 'alphatest.png': %s" % (str(e), ))

                self.image_stream.close()
                self.image_stream = None
                self.load_timeout = 0

                dialog.show()
                dialog.connect('response', lambda x, y: dialog.destroy())

                return False  # uninstall the timeout

            try:
                self.pixbuf_loader.write(buf)

            except GObject.GError as e:
                dialog = Gtk.MessageDialog(self.window,
                                           Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                           Gtk.MessageType.ERROR,
                                           Gtk.ButtonsType.CLOSE,
                                           e.message)

                self.image_stream.close()
                self.image_stream = None
                self.load_timeout = 0

                dialog.show()
                dialog.connect('response', lambda x, y: dialog.destroy())

                return False  # uninstall the timeout

            if len(buf) < 256:  # eof
                self.image_stream.close()
                self.image_stream = None

                # Errors can happen on close, e.g. if the image
                # file was truncated we'll know on close that
                # it was incomplete.
                try:
                    self.pixbuf_loader.close()
                except GObject.GError as e:
                    dialog = Gtk.MessageDialog(self.window,
                                               Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                               Gtk.MessageType.ERROR,
                                               Gtk.ButtonsType.CLOSE,
                                               'Failed to load image: %s' % e.message)

                    self.load_timeout = 0

                    dialog.show()
                    dialog.connect('response', lambda x, y: dialog.destroy())

                    return False  # uninstall the timeout
        else:
            img_path = path.join(self.base_path, 'alphatest.png')
            try:
                self.image_stream = open(img_path, 'rb')
            except IOError as e:
                dialog = Gtk.MessageDialog(self.window,
                                           Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                           Gtk.MessageType.ERROR,
                                           Gtk.ButtonsType.CLOSE,
                                           str(e))
                self.load_timeout = 0
                dialog.show()
                dialog.connect('response', lambda x, y: dialog.destroy())

                return False  # uninstall the timeout

            if self.pixbuf_loader:
                try:
                    self.pixbuf_loader.close()
                except GObject.GError:
                    pass
                self.pixbuf_loader = None

            self.pixbuf_loader = GdkPixbuf.PixbufLoader()

            self.pixbuf_loader.connect('area-prepared',
                                       self.progressive_prepared_callback,
                                       image)

            self.pixbuf_loader.connect('area-updated',
                                       self.progressive_updated_callback,
                                       image)
        # leave timeout installed
        return True

    def progressive_prepared_callback(self, loader, image):
        pixbuf = loader.get_pixbuf()
        # Avoid displaying random memory contents, since the pixbuf
        # isn't filled in yet.
        pixbuf.fill(0xaaaaaaff)
        image.set_from_pixbuf(pixbuf)

    def progressive_updated_callback(self, loader, x, y, width, height, image):
        # We know the pixbuf inside the GtkImage has changed, but the image
        # itself doesn't know this; so queue a redraw.  If we wanted to be
        # really efficient, we could use a drawing area or something
        # instead of a GtkImage, so we could control the exact position of
        # the pixbuf on the display, then we could queue a draw for only
        # the updated area of the image.
        image.queue_draw()

    def start_progressive_loading(self, image):
        # This is obviously totally contrived (we slow down loading
        # on purpose to show how incremental loading works).
        # The real purpose of incremental loading is the case where
        # you are reading data from a slow source such as the network.
        # The timeout simply simulates a slow data source by inserting
        # pauses in the reading process.

        self.load_timeout = Gdk.threads_add_timeout(150,
                                                    GLib.PRIORITY_DEFAULT_IDLE,
                                                    self.progressive_timeout,
                                                    image)

    def cleanup_cb(self, widget):
        if self.load_timeout:
            GLib.source_remove(self.load_timeout)

        if self.pixbuf_loader:
            try:
                self.pixbuf_loader.close()
            except GObject.GError:
                pass

        if self.image_stream:
            self.image_stream.close()

        Gtk.main_quit()


def main(demoapp=None):
    ImagesApp()
    Gtk.main()

if __name__ == '__main__':
    main()
