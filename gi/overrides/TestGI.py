# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Simon van der Linden <svdlinden@src.gnome.org>
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

from ..types import override
from ..importer import modules

TestGI = modules['TestGI']


OVERRIDES_CONSTANT = 7


class OverridesStruct(TestGI.OverridesStruct):

    def __new__(cls, long_):
        return TestGI.OverridesStruct.__new__(cls)

    def __init__(self, long_):
        TestGI.OverridesStruct.__init__(self)
        self.long_ = long_

    def method(self):
        return TestGI.OverridesStruct.method(self) / 7

OverridesStruct = override(OverridesStruct)


class OverridesObject(TestGI.OverridesObject):

    def __new__(cls, long_):
        return TestGI.OverridesObject.__new__(cls)

    def __init__(self, long_):
        TestGI.OverridesObject.__init__(self)
        # FIXME: doesn't work yet
        #self.long_ = long_

    @classmethod
    def new(cls, long_):
        self = TestGI.OverridesObject.new()
        # FIXME: doesn't work yet
        #self.long_ = long_
        return self

    def method(self):
        return TestGI.OverridesObject.method(self) / 7

OverridesObject = override(OverridesObject)


__all__ = [OVERRIDES_CONSTANT, OverridesStruct, OverridesObject]

