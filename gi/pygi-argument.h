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

#include "pygi-cache.h"

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
gboolean _pygi_marshal_in_void        (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_boolean     (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int8        (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint8       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int16       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint16      (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int32       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint32      (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_int64       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_uint64      (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_float       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_double      (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_unichar     (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_gtype       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_utf8        (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_filename    (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_array       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_glist       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_gslist      (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_ghash       (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_gerror      (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);
gboolean _pygi_marshal_in_interface_callback (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_enum     (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_flags    (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_struct   (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_interface(PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_boxed    (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_object   (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);
gboolean _pygi_marshal_in_interface_union    (PyGIState         *state,
                                              PyGIFunctionCache *function_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg);

G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */
