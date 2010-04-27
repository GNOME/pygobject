# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2009 Johan Dahlin <johan@gnome.org>
#               2010 Simon van der Linden <svdlinden@src.gnome.org>
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

import gobject
from gi.repository import Gdk
from gi.repository import GObject
from ..types import override
from ..importer import modules

Gtk = modules['Gtk']

class Builder(Gtk.Builder):

    def connect_signals(self, obj_or_map):
        def _full_callback(builder, gobj, signal_name, handler_name, connect_obj, flags, obj_or_map):
            handler = None
            if isinstance(obj_or_map, dict):
                handler = obj_or_map.get(handler_name, None)
            else:
                handler = getattr(obj_or_map, handler_name, None)

            if handler is None:
                raise AttributeError('Handler %s not found' % handler_name)

            if not callable(handler):
                raise TypeError('Handler %s is not a method or function' % handler_name)

            # TODO: we need to support bitfields
            after = False #flags and GObject.ConnectFlags.AFTER
            if connect_obj is not None:
                if after:
                    gobj.connect_object_after(signal_name, handler, connect_obj)
                else:
                    gobj.connect_object(signal_name, handler, connect_obj)
            else:
                if after:
                    gobj.connect_after(signal_name, handler)
                else:
                    gobj.connect(signal_name, handler)

        self.connect_signals_full(_full_callback,
                                  obj_or_map);

Builder = override(Builder)
__all__ = ['Builder']

import sys

initialized, argv = Gtk.init_check(sys.argv)
if not initialized:
    raise RuntimeError("Gtk couldn't be initialized")
