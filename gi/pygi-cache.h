/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
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

#ifdef ENABLE_INVOKE_NG
#ifndef __PYGI_CACHE_H__
#define __PYGI_CACHE_H__

#include <Python.h>
#include <girepository.h>

#include "pygi-invoke-state-struct.h"

G_BEGIN_DECLS

typedef struct _PyGIFunctionCache PyGIFunctionCache;
typedef struct _PyGIArgCache PyGIArgCache;

typedef gboolean (*PyGIMarshalInFunc) (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);

typedef PyObject *(*PyGIMarshalOutFunc) (PyGIInvokeState   *state,
                                         PyGIFunctionCache *function_cache,
                                         PyGIArgCache      *arg_cache,
                                         GIArgument        *arg);

typedef enum {
  /* Not an AUX type */
  PYGI_AUX_TYPE_NONE   = 0,
  /* AUX type handled by parent */
  PYGI_AUX_TYPE_IGNORE = 1,
  /* AUX type has an associated pyarg which is modified by parent */
  PYGI_AUX_TYPE_HAS_PYARG = 2
} PyGIAuxType;

struct _PyGIArgCache
{
    PyGIAuxType aux_type;
    gboolean is_pointer;
    gboolean is_caller_allocates;
    gboolean allow_none;

    GIDirection direction;
    GITransfer transfer;
    GITypeTag type_tag;
    GITypeInfo *type_info;
    GIArgument *default_value;

    PyGIMarshalInFunc in_marshaller;
    PyGIMarshalOutFunc out_marshaller;
    GDestroyNotify cleanup;

    GDestroyNotify destroy_notify;

    gssize c_arg_index;
    gssize py_arg_index;
};

typedef struct _PyGISequenceCache
{
    PyGIArgCache arg_cache;
    gssize fixed_size;
    gint len_arg_index;
    gboolean is_zero_terminated;
    gsize item_size;
    GIArrayType array_type;
    PyGIArgCache *item_cache;
} PyGISequenceCache;

typedef struct _PyGIInterfaceCache
{
    PyGIArgCache arg_cache;
    gboolean is_foreign;
    GType g_type;
    PyObject *py_type;
    GIInterfaceInfo *interface_info;
    gchar *type_name;
} PyGIInterfaceCache;

typedef struct _PyGIHashCache
{
    PyGIArgCache arg_cache;
    PyGIArgCache *key_cache;
    PyGIArgCache *value_cache;
} PyGIHashCache;

typedef struct _PyGICallbackCache
{
    PyGIArgCache arg_cache;
    gint user_data_index;
    gint destroy_notify_index;
    GIScopeType scope;
    GIInterfaceInfo *interface_info;
} PyGICallbackCache;

struct _PyGIFunctionCache
{
    const gchar *name;

    gboolean is_method;
    gboolean is_constructor;
    gboolean is_vfunc;
    gboolean is_callback;

    PyGIArgCache *return_cache;
    PyGIArgCache **args_cache;
    GSList *out_args;

    /* counts */
    gssize n_in_args;
    gssize n_out_args;
    gssize n_out_aux_args;

    gssize n_args;
    gssize n_py_args;
};

void _pygi_arg_cache_clear	(PyGIArgCache *cache);
void _pygi_function_cache_free	(PyGIFunctionCache *cache);

PyGIFunctionCache *_pygi_function_cache_new (GIFunctionInfo *function_info);

G_END_DECLS

#endif /* __PYGI_CACHE_H__ */
#endif /* def ENABLE_INVOKE_NG */
