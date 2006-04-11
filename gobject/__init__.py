# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2006  Johan Dahlin
#
#   gobject/__init__.py: initialisation file for gobject module
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

# this can go when things are a little further along
try:
    import ltihooks
    ltihooks # pyflakes
    del ltihooks
except ImportError:
    pass

from _gobject import *

class GObjectMeta(type):
    "Metaclass for automatically gobject.type_register()ing GObject classes"
    def __init__(cls, name, bases, dict_):
        type.__init__(cls, name, bases, dict_)
        ## don't register the class if already registered
        if '__gtype__' in cls.__dict__:
            return

        if not ('__gproperties__' in cls.__dict__ or
                '__gsignals__' in cls.__dict__ or
                '__gtype_name__' in cls.__dict__):
            return

        type_register(cls, cls.__dict__.get('__gtype_name__'))

_gobject._install_metaclass(GObjectMeta)

