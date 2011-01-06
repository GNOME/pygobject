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

G_BEGIN_DECLS

typedef struct _PyGIFunctionCache PyGIFunctionCache;
typedef struct _PyGIArgCache PyGIArgCache;

typedef gboolean (*PyGIMarshalInFunc) (PyGIState         *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);

typedef gboolean (*PyGIMarshalOutFunc) (void);
typedef gboolean (*PyGIArgCleanupFunc) (gpointer data);


typedef struct _PyGISequenceCache
{
    gssize fixed_size;
    PyGIValidateFunc *item_validate_func;
    PyGIMarshalFunc *item_marshal_func;
    gint len_arg_index;
    gboolean is_zero_terminated;
    gsize item_size;
    GITypeTag item_tag_type;
} PyGISequenceCache;

typedef struct _PyGIInterfaceCache
{
    gboolean is_foreign;
    GType g_type;
    PyObject *py_type;
} PyGIInterfaceCache;

typedef struct _PyGIHashCache
{
    GITypeTag key_type_tag;
    PyGIValidateFunc *key_validate_func;
    PyGIMarshalFunc *key_marshal_func;
    GITypeTag value_type_tag;
    PyGIValidateFunc *value_validate_func;
    PyGIValidateFunc *value_marshal_func;
} PyGIHashCache;

typedef struct _PyGICallbackCache
{
    gint py_user_data_index;
    gint user_data_index;
    gint destroy_notify_index;
    GScope scope;
} PyGICallbackCache;

struct _PyGIArgCache
{
    gboolean is_aux;
    gboolean is_pointer;
    GIDirection direction;
    GITransfer transfer;
    GIArgInfo *arg_info;
    GIArgument *default_value;

    PyGIMashalInFunc in_marshaler;
    PyGIMarshalOutFunc out_marshaler;
    PyGIArgCleanupFunc cleanup;

    PyGISequenceCache *sequence_cache;
    PyGIInterfaceCache *interface_cache;
    PyGIHashCache *hash_cache;
    PyCallbackCache *callback_cache;

    gint c_arg_index;
    gint py_arg_index;
};

struct _PyGIFunctionCache
{
    gboolean is_method;
    gboolean is_constructor;

    PyGIArgCache **args_cache;
    GSList *in_args;
    GSList *out_arg;

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
