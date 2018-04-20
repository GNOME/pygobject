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
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

import sys

PY2 = PY3 = False
if sys.version_info[0] == 2:
    PY2 = True

    from StringIO import StringIO
    StringIO

    from UserList import UserList
    UserList

    long_ = eval("long")
    integer_types = eval("(int, long)")
    string_types = eval("(basestring,)")
    text_type = eval("unicode")

    reload = eval("reload")
    xrange = eval("xrange")
    cmp = eval("cmp")

    exec("def reraise(tp, value, tb):\n raise tp, value, tb")
else:
    PY3 = True

    from io import StringIO
    StringIO

    from collections import UserList
    UserList

    long_ = int
    integer_types = (int,)
    string_types = (str,)
    text_type = str

    from importlib import reload
    reload
    xrange = range
    cmp = lambda a, b: (a > b) - (a < b)

    def reraise(tp, value, tb):
        raise tp(value).with_traceback(tb)
