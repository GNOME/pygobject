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

struct _PyGIArgCache
{
    gboolean is_aux;
    gboolean is_pointer;
    gboolean is_caller_allocates;

    GIDirection direction;
    GITransfer transfer;
    GITypeTag type_tag;
    GIArgInfo *arg_info;
    GIArgument *default_value;

    PyGIMarshalInFunc in_marshaller;
    PyGIMarshalOutFunc out_marshaller;
    GDestroyNotify cleanup;

    GDestroyNotify destroy_notify;

    gint c_arg_index;
    gint py_arg_index;
};

typedef struct _PyGISequenceCache
{
    PyGIArgCache arg_cache;
    gssize fixed_size;
    gint len_arg_index;
    gboolean is_zero_terminated;
    gsize item_size;
    PyGIArgCache *item_cache;
} PyGISequenceCache;

typedef struct _PyGIInterfaceCache
{
    PyGIArgCache arg_cache;
    gboolean is_foreign;
    GType g_type;
    PyObject *py_type;
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
    gint py_user_data_index;
    gint user_data_index;
    gint destroy_notify_index;
    GIScopeType scope;
} PyGICallbackCache;

struct _PyGIFunctionCache
{
    gboolean is_method;
    gboolean is_constructor;

    PyGIArgCache *return_cache;
    PyGIArgCache **args_cache;
    GSList *in_args;
    GSList *out_args;

    /* counts */
    guint n_in_args;
    guint n_out_args;
    guint n_args;
};

void _pygi_arg_cache_clear	(PyGIArgCache *cache);
void _pygi_function_cache_free	(PyGIFunctionCache *cache);

PyGIFunctionCache *_pygi_function_cache_new (GIFunctionInfo *function_info);

G_END_DECLS

#endif /* __PYGI_CACHE_H__ */
