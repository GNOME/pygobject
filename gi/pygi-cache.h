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

typedef struct _PyGICallableCache PyGICallableCache;
typedef struct _PyGIArgCache PyGIArgCache;

typedef gboolean (*PyGIMarshalInFunc) (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg);

typedef PyObject *(*PyGIMarshalOutFunc) (PyGIInvokeState   *state,
                                         PyGICallableCache *callable_cache,
                                         PyGIArgCache      *arg_cache,
                                         GIArgument        *arg);

typedef void (*PyGIMarshalCleanupFunc) (PyGIInvokeState *state,
                                        PyGIArgCache    *arg_cache,
                                        gpointer         data,
                                        gboolean         was_processed);

/* Argument meta types denote how we process the argument:
 *  - Parents (PYGI_META_ARG_TYPE_PARENT) may or may not have children
 *    but are always processed via the normal marshaller for their
 *    actual GI type.  If they have children the marshaller will
 *    also handle marshalling the children.
 *  - Children without python argument (PYGI_META_ARG_TYPE_CHILD) are
 *    ignored by the marshallers and handled directly by their parents
 *    marshaller.
 *  - Children with pyargs (PYGI_META_ARG_TYPE_CHILD_WITH_PYARG) are processed
 *    the same as other child args but also have an index into the 
 *    python parameters passed to the invoker
 */
typedef enum {
    PYGI_META_ARG_TYPE_PARENT,
    PYGI_META_ARG_TYPE_CHILD,
    PYGI_META_ARG_TYPE_CHILD_WITH_PYARG
} PyGIMetaArgType;


struct _PyGIArgCache
{
    PyGIMetaArgType meta_type;
    gboolean is_pointer;
    gboolean is_caller_allocates;
    gboolean allow_none;

    GIDirection direction;
    GITransfer transfer;
    GITypeTag type_tag;
    GITypeInfo *type_info;

    PyGIMarshalInFunc in_marshaller;
    PyGIMarshalOutFunc out_marshaller;

    PyGIMarshalCleanupFunc in_cleanup;
    PyGIMarshalCleanupFunc out_cleanup;

    GDestroyNotify destroy_notify;

    gssize c_arg_index;
    gssize py_arg_index;
};

typedef struct _PyGISequenceCache
{
    PyGIArgCache arg_cache;
    gssize fixed_size;
    gssize len_arg_index;
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
    gssize user_data_index;
    gssize destroy_notify_index;
    GIScopeType scope;
    GIInterfaceInfo *interface_info;
} PyGICallbackCache;

struct _PyGICallableCache
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
    gssize n_out_child_args;

    gssize n_args;
    gssize n_py_args;
};

void _pygi_arg_cache_clear	(PyGIArgCache *cache);
void _pygi_callable_cache_free	(PyGICallableCache *cache);

PyGICallableCache *_pygi_callable_cache_new (GICallableInfo *callable_info);

G_END_DECLS

#endif /* __PYGI_CACHE_H__ */
#endif /* def ENABLE_INVOKE_NG */
