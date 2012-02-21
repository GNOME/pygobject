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

typedef struct _PyGICallableCache PyGICallableCache;
typedef struct _PyGIArgCache PyGIArgCache;

typedef gboolean (*PyGIMarshalFromPyFunc) (PyGIInvokeState   *state,
                                           PyGICallableCache *callable_cache,
                                           PyGIArgCache      *arg_cache,
                                           PyObject          *py_arg,
                                           GIArgument        *arg);

typedef PyObject *(*PyGIMarshalToPyFunc) (PyGIInvokeState   *state,
                                          PyGICallableCache *callable_cache,
                                          PyGIArgCache      *arg_cache,
                                          GIArgument        *arg);

typedef void (*PyGIMarshalCleanupFunc) (PyGIInvokeState *state,
                                        PyGIArgCache    *arg_cache,
                                        gpointer         data,
                                        gboolean         was_processed);

/* Argument meta types denote how we process the argument:
 *  - PYGI_META_ARG_TYPE_PARENT - parents may or may not have children
 *    but are always processed via the normal marshaller for their
 *    actual GI type.  If they have children the marshaller will
 *    also handle marshalling the children.
 *  - PYGI_META_ARG_TYPE_CHILD - Children without python argument are
 *    ignored by the marshallers and handled directly by their parents
 *    marshaller.
 *  - PYGI_META_ARG_TYPE_CHILD_NEEDS_UPDATE -Sometimes children arguments
 *    come before the parent.  In these cases they need to be flagged
 *    so that the argument list counts must be updated for the cache to
 *    be valid
 *  - Children with pyargs (PYGI_META_ARG_TYPE_CHILD_WITH_PYARG) are processed
 *    the same as other child args but also have an index into the 
 *    python parameters passed to the invoker
 */
typedef enum {
    PYGI_META_ARG_TYPE_PARENT,
    PYGI_META_ARG_TYPE_CHILD,
    PYGI_META_ARG_TYPE_CHILD_NEEDS_UPDATE,
    PYGI_META_ARG_TYPE_CHILD_WITH_PYARG,
    PYGI_META_ARG_TYPE_CLOSURE,
} PyGIMetaArgType;

/*
 * GI determines function types via a combination of flags and info classes.
 * Since for branching purposes they are mutually exclusive, the 
 * PyGIFunctionType enum consolidates them into one enumeration for ease of 
 * branching and debugging.
 */
typedef enum {
    PYGI_FUNCTION_TYPE_FUNCTION,
    PYGI_FUNCTION_TYPE_METHOD,
    PYGI_FUNCTION_TYPE_CONSTRUCTOR,
    PYGI_FUNCTION_TYPE_VFUNC,
    PYGI_FUNCTION_TYPE_CALLBACK,
    PYGI_FUNCTION_TYPE_CCALLBACK,
 } PyGIFunctionType;

/*
 * In PyGI IN and OUT arguments mean different things depending on the context
 * of the callable (e.g. is it a callback that is being called from C or a
 * function that is being called from python).  We don't as much care if the
 * parameter is an IN or OUT C parameter, than we do if the parameter is being
 * marshalled into Python or from Python.
 */
typedef enum {
    PYGI_DIRECTION_TO_PYTHON,
    PYGI_DIRECTION_FROM_PYTHON,
    PYGI_DIRECTION_BIDIRECTIONAL
 } PyGIDirection;


struct _PyGIArgCache
{
    const gchar *arg_name;

    PyGIMetaArgType meta_type;
    gboolean is_pointer;
    gboolean is_caller_allocates;
    gboolean is_skipped;
    gboolean allow_none;

    PyGIDirection direction;
    GITransfer transfer;
    GITypeTag type_tag;
    GITypeInfo *type_info;

    PyGIMarshalFromPyFunc from_py_marshaller;
    PyGIMarshalToPyFunc to_py_marshaller;

    PyGIMarshalCleanupFunc from_py_cleanup;
    PyGIMarshalCleanupFunc to_py_cleanup;

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

    PyGIFunctionType function_type;

    PyGIArgCache *return_cache;
    PyGIArgCache **args_cache;
    GSList *to_py_args;
    GSList *arg_name_list; /* for keyword arg matching */
    GHashTable *arg_name_hash;

    /* counts */
    gssize n_from_py_args;
    gssize n_to_py_args;
    gssize n_to_py_child_args;

    gssize n_args;
    gssize n_py_args;
};

void _pygi_arg_cache_clear	(PyGIArgCache *cache);
void _pygi_callable_cache_free	(PyGICallableCache *cache);

PyGICallableCache *_pygi_callable_cache_new (GICallableInfo *callable_info,
                                             gboolean is_ccallback);

G_END_DECLS

#endif /* __PYGI_CACHE_H__ */
