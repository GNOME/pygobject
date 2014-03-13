/* -*- Mode: C; c-basic-offset: 4 -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
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

#ifndef __PYG_OPTIONCONTEXT_H__
#define __PYG_OPTIONCONTEXT_H__

#include "pygoptiongroup.h"

extern PyTypeObject PyGOptionContext_Type;

typedef struct {
    PyObject_HEAD
    PyGOptionGroup *main_group;
    GOptionContext *context;
} PyGOptionContext;

PyObject* pyg_option_context_new(GOptionContext *context);

void pyglib_option_context_register_types(PyObject *d);

#endif /* __PYG_OPTIONCONTEXT_H__ */
