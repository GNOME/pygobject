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

title = "Drawing Area"
description = """
GtkDrawingArea is a blank area where you can draw custom displays
of various kinds.

This demo has two drawing areas. The checkerboard area shows
how you can just draw something; all you have to do is write
a signal handler for expose_event, as shown here.

The "scribble" area is a bit more advanced, and shows how to handle
events such as button presses and mouse motion. Click the mouse
and drag in the scribble area to draw squiggles. Resize the window
to clear the area.
"""

# See FIXME's
is_fully_bound = False

from gi.repository import Gtk, Gdk

class DrawingAreaApp:
    def __init__(self):
        self.pixmap = None

        window = Gtk.Window()
        window.set_title(title)
        window.connect('destroy', lambda x: Gtk.main_quit())
        window.set_border_width(8)

        vbox = Gtk.VBox(homogeneous=False, spacing=8)
        window.add(vbox)

        # create checkerboard area
        label = Gtk.Label()
        label.set_markup('<u>Scribble area</u>')
        vbox.pack_start(label, False, False, 0)

        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)
        vbox.pack_start(frame, True, True, 0)

        da = Gtk.DrawingArea()
        da.set_size_request(100, 100)
        frame.add(da)
        da.connect('expose-event', self.checkerboard_expose_event)

        # create scribble area
        label = Gtk.Label()
        label.set_markup('<u>Checkerboard pattern</u>')
        vbox.pack_start(label, False, False, 0)

        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)
        vbox.pack_start(frame, True, True, 0)

        da = Gtk.DrawingArea()
        da.set_size_request(100, 100)
        frame.add(da)
        da.connect('expose-event', self.scribble_expose_event)
        da.connect('configure-event', self.scribble_configure_event)

        # event signals
        da.connect('motion-notify-event', self.scribble_motion_notify_event)
        da.connect('button-press-event', self.scribble_button_press_event)

        # Ask to receive events the drawing area doesn't normally
        # subscribe to
        da.set_events(da.get_events()
                      | Gdk.EventMask.LEAVE_NOTIFY_MASK
                      | Gdk.EventMask.BUTTON_PRESS_MASK
                      | Gdk.EventMask.POINTER_MOTION_MASK
                      | Gdk.EventMask.POINTER_MOTION_HINT_MASK)

        window.show_all()

    def checkerboard_expose_event(self, da, event):
        check_size = 10
        spacing = 2

        # At the start of an expose handler, a clip region of event->area
        # is set on the window, and event->area has been cleared to the
        # widget's background color. The docs for
        # gdk_window_begin_paint_region() give more details on how this
        # works.

        # It would be a bit more efficient to keep these
        # GC's around instead of recreating on each expose, but
        # this is the lazy/slow way.

        gc1 = Gdk.GC.new(da.get_window())
        color = Gdk.Color(30000, 0, 30000)
        gc1.set_rgb_fg_color(color)

        gc2 = Gdk.GC.new(da.get_window())
        color = Gdk.Color(65535, 65535, 65535)
        gc2.set_rgb_fg_color(color)

        xcount = 0
        i = spacing

        while i < da.allocation.width:
            j = spacing
            ycount = xcount % 2 # start with even/odd depending on row
            while j < da.allocation.height:
                if ycount % 2:
                    gc = gc1
                else:
                    gc = gc2

                # If we're outside event->area, this will do nothing.
                # It might be mildly more efficient if we handled
                # the clipping ourselves, but again we're feeling lazy.
                Gdk.draw_rectangle(da.get_window(),
                                   gc,
                                   True,
                                   i, j,
                                   check_size,
                                   check_size)

                j += check_size + spacing
                ycount += 1

            i += check_size + spacing
            xcount += 1

        return True

    def scribble_expose_event(self, da, event):

        # We use the "foreground GC" for the widget since it already exists,
        # but honestly any GC would work. The only thing to worry about
        # is whether the GC has an inappropriate clip region set.

        # FIXME: we should be using widget.style.forground_gc but
        # there is a bug in GObject Introspection when getting
        # certain struct offsets so we create a new GC here
        black_gc = Gdk.GC.new(da.get_window())
        color = Gdk.Color(0, 0, 0)
        black_gc.set_rgb_fg_color(color)

        Gdk.draw_drawable(da.get_window(),
                          black_gc,
                          self.pixmap,
                          event.expose.area.x, event.expose.area.y,
                          event.expose.area.x, event.expose.area.y,
                          event.expose.area.width, event.expose.area.height)

        return False

    def draw_brush(self, widget, x, y):
        update_rect = Gdk.Rectangle(x - 3,
                                    y - 3,
                                    6,
                                    6)

        # FIXME: we should be using widget.style.black_gc but
        # there is a bug in GObject Introspection when getting
        # certain struct offsets so we create a new GC here
        black_gc = Gdk.GC.new(widget.get_window())
        color = Gdk.Color(0, 0, 0)
        black_gc.set_rgb_fg_color(color)

        Gdk.draw_rectangle(self.pixmap,
                           black_gc,
                           True,
                           update_rect.x, update_rect.y,
                           update_rect.width, update_rect.height)

        widget.get_window().invalidate_rect(update_rect, False)

    def scribble_configure_event(self, da, event):
        self.pixmap = Gdk.Pixmap.new(da.get_window(),
                                     da.allocation.width,
                                     da.allocation.height,
                                     -1)

        # FIXME: we should be using widget.style.white_gc but
        # there is a bug in GObject Introspection when getting
        # certain struct offsets so we create a new GC here
        white_gc = Gdk.GC.new(da.get_window())
        color = Gdk.Color(65535, 65535, 65535)
        white_gc.set_rgb_fg_color(color)
        Gdk.draw_rectangle(self.pixmap,
                           white_gc,
                           True,
                           0, 0,
                           da.allocation.width,
                           da.allocation.height)

        return True

    def scribble_motion_notify_event(self, da, event):
        if self.pixmap == None: # paranoia check, in case we haven't gotten a configure event
            return False

        # This call is very important; it requests the next motion event.
        # If you don't call gdk_window_get_pointer() you'll only get
        # a single motion event. The reason is that we specified
        # GDK_POINTER_MOTION_HINT_MASK to gtk_widget_set_events().
        # If we hadn't specified that, we could just use event->x, event->y
        # as the pointer location. But we'd also get deluged in events.
        # By requesting the next event as we handle the current one,
        # we avoid getting a huge number of events faster than we
        # can cope.

        (window, x, y, state) = event.motion.window.get_pointer()

        # FIXME: for some reason Gdk.ModifierType.BUTTON1_MASK doesn't exist
        #if state & Gdk.ModifierType.BUTTON1_MASK:
        #    self.draw_brush(da, x, y)

        return True

    def scribble_button_press_event(self, da, event):
        if self.pixmap == None: # paranoia check, in case we haven't gotten a configure event
            return False

        if event.button.button == 1:
            self.draw_brush(da, event.button.x, event.button.y)

        return True

def main():
    app = DrawingAreaApp()
    Gtk.main()

if __name__ == '__main__':
    main()
