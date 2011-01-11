/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifndef __PYGI_ARGUMENT_H__
#define __PYGI_ARGUMENT_H__

#include <Python.h>

#include <girepository.h>

#include "pygi-private.h"

G_BEGIN_DECLS

/* Private */
gint _pygi_g_type_interface_check_object (GIBaseInfo *info,
                                          PyObject   *object);

gint _pygi_g_type_info_check_object (GITypeInfo *type_info,
                                     PyObject   *object,
                                     gboolean   allow_none);

gint _pygi_g_registered_type_info_check_object (GIRegisteredTypeInfo *info,
                                                gboolean              is_instance,
                                                PyObject             *object);


GArray* _pygi_argument_to_array (GIArgument  *arg,
                                 GIArgument  *args[],
                                 GITypeInfo *type_info,
                                 gboolean    is_method);

GIArgument _pygi_argument_from_object (PyObject   *object,
                                      GITypeInfo *type_info,
                                      GITransfer  transfer);

PyObject* _pygi_argument_to_object (GIArgument  *arg,
                                    GITypeInfo *type_info,
                                    GITransfer  transfer);


void _pygi_argument_release (GIArgument   *arg,
                             GITypeInfo  *type_info,
                             GITransfer   transfer,
                             GIDirection  direction);

void _pygi_argument_init (void);


/*** argument marshaling and validating routines ***/
gboolean _pygi_marshal_in_void        (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_boolean     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int8        (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint8       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int16       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint16      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int32       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint32      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int64       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint64      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_float       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_double      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_unichar     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_gtype       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_utf8        (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_filename    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_array       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_glist       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_gslist      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_ghash       (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_gerror      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_interface_callback (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_enum     (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_flags    (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_struct   (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_interface(PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_boxed    (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_object   (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_union    (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);

PyObject *_pygi_marshal_out_void      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_boolean   (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_int8      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_uint8     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_int16     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_uint16    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_int32     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_uint32    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_int64     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_uint64    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_float     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_double    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_unichar   (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_gtype     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_utf8      (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_filename  (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_array     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_glist     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_gslist    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_ghash     (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_gerror    (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_callback(PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_enum   (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_flags  (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_struct (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_interface(PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_boxed  (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_object (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);
PyObject *_pygi_marshal_out_interface_union  (PyGIInvokeState   *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg);

G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */
