/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
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

#ifndef __PYGI_VALUE_H__
#define __PYGI_VALUE_H__

#include <glib-object.h>
#include <girepository.h>
#include <Python.h>

G_BEGIN_DECLS

GIArgument _pygi_argument_from_g_value(const GValue *value,
                                       GITypeInfo *type_info);

int       pyg_value_from_pyobject(GValue *value, PyObject *obj);
int       pyg_value_from_pyobject_with_error(GValue *value, PyObject *obj);
PyObject *pyg_value_as_pyobject(const GValue *value, gboolean copy_boxed);
int       pyg_param_gvalue_from_pyobject(GValue* value,
                                         PyObject* py_obj,
                                         const GParamSpec* pspec);
PyObject *pyg_param_gvalue_as_pyobject(const GValue* gvalue,
                                       gboolean copy_boxed,
                                       const GParamSpec* pspec);
PyObject *pyg_strv_from_gvalue(const GValue *value);
int       pyg_strv_to_gvalue(GValue *value, PyObject *obj);

PyObject *pygi_value_to_py_basic_type      (const GValue *value,
                                            GType fundamental);
PyObject *pygi_value_to_py_structured_type (const GValue *value,
                                            GType fundamental,
                                            gboolean copy_boxed);

G_END_DECLS

#endif /* __PYGI_VALUE_H__ */
