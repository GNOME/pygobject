# -*- Mode: Python; py-indent-offset: 4 -*-
# pygtk - Python bindings for the GTK+ widget set.
# Copyright (C) 1998-2002  James Henstridge
#
#   pygtk.py: pygtk version selection code.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

import sys, os, re

__all__ = ['require']

_pygtk_dir_pat = re.compile(r'^gtk-([\d]+[\d\.]+)$')

def _get_available_versions():
    versions = {}
    for dir in sys.path:
        if not dir: dir = os.getcwd()
        if not os.path.exists(dir): continue
        if _pygtk_dir_pat.match(os.path.basename(dir)):
            continue  # if the dir is a pygtk dir, skip it
        for filename in os.listdir(dir):
            pathname = os.path.join(dir, filename)
            if not os.path.isdir(pathname):
                continue  # skip non directories
            match = _pygtk_dir_pat.match(filename)
            if match and not versions.has_key(match.group(1)):
                versions[match.group(1)] = pathname
    return versions

def require(version):
    assert not sys.modules.has_key('gtk'), \
           "pygtk.require() must be called before importing gtk"

    versions = _get_available_versions()
    assert versions.has_key(version), \
           "required version '%s' not found on system" % version

    # remove any pygtk dirs first ...
    for dir in sys.path:
        if _pygtk_dir_pat.match(os.path.basename(dir)):
            sys.path.remove(dir)

    # prepend the pygtk path ...
    sys.path.insert(0, versions[version])
