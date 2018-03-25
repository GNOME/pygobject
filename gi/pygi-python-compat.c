/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pygi-python-compat.h"

/* Better alternative to PyImport_ImportModule which tries to import from
 * sys.modules first */
PyObject *
PYGLIB_PyImport_ImportModule(const char *name)
{
#if PY_VERSION_HEX < 0x03000000 && !defined(PYPY_VERSION)
    /* see PyImport_ImportModuleNoBlock
     * https://github.com/python/cpython/blob/2.7/Python/import.c#L2166-L2206 */
    PyObject *result = PyImport_ImportModuleNoBlock(name);
    if (result)
        return result;

    PyErr_Clear();
#endif
    return PyImport_ImportModule(name);
}
