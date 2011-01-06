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
PyGIMarshalInFunc _pygi_marshal_in_void;
PyGIMarshalInFunc _pygi_marshal_in_int8;
PyGIMarshalInFunc _pygi_marshal_in_uint8;
PyGIMarshalInFunc _pygi_marshal_in_int16;
PyGIMarshalInFunc _pygi_marshal_in_uint16;
PyGIMarshalInFunc _pygi_marshal_in_int32;
PyGIMarshalInFunc _pygi_marshal_in_uint32;
PyGIMarshalInFunc _pygi_marshal_in_int64;
PyGIMarshalInFunc _pygi_marshal_in_float;
PyGIMarshalInFunc _pygi_marshal_in_double;
PyGIMarshalInFunc _pygi_marshal_in_unichar;
PyGIMarshalInFunc _pygi_marshal_in_gtype;
PyGIMarshalInFunc _pygi_marshal_in_utf8;
PyGIMarshalInFunc _pygi_marshal_in_filename;
PyGIMarshalInFunc _pygi_marshal_in_array;
PyGIMarshalInFunc _pygi_marshal_in_glist;
PyGIMarshalInFunc _pygi_marshal_in_gslist;
PyGIMarshalInFunc _pygi_marshal_in_ghash;
PyGIMarshalInFunc _pygi_marshal_in_gerror;
PyGIMarshalInFunc _pygi_marshal_in_interface_callback;
PyGIMarshalInFunc _pygi_marshal_in_interface_enum;
PyGIMarshalInFunc _pygi_marshal_in_interface_flags;
PyGIMarshalInFunc _pygi_marshal_in_interface_struct;
PyGIMarshalInFunc _pygi_marshal_in_interface_interface;
PyGIMarshalInFunc _pygi_marshal_in_interface_boxed;
PyGIMarshalInFunc _pygi_marshal_in_interface_object;
PyGIMarshalInFunc _pygi_marshal_in_interface_union;



G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */
