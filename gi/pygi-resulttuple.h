/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2015 Christoph Reiter <reiter.christoph@gmail.com>
 *
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

#ifndef __PYGI_RESULTTUPLE_H__
#define __PYGI_RESULTTUPLE_H__

#include "Python.h"

int pygi_resulttuple_register_types (PyObject *d);

PyTypeObject *pygi_resulttuple_new_type (PyObject *tuple_names);

PyObject *pygi_resulttuple_new (PyTypeObject *subclass, Py_ssize_t len);

#endif /* __PYGI_RESULTTUPLE_H__ */
