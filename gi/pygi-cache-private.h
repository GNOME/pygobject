/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#ifndef __PYGI_CACHE_PRIVATE_H__
#define __PYGI_CACHE_PRIVATE_H__

#include "pygi-cache.h"

G_BEGIN_DECLS


PyGIArgCache *pygi_arg_cache_alloc (void);

PyGIArgCache *pygi_arg_cache_new (GITypeInfo *type_info, GIArgInfo *arg_info,
                                  GITransfer transfer, PyGIDirection direction,
                                  PyGICallableCache *callable_cache,
                                  gssize c_arg_index, gssize py_arg_index);

void pygi_arg_cache_free (PyGIArgCache *cache);

void pygi_arg_base_setup (
    PyGIArgCache *arg_cache, GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction);

gboolean pygi_arg_sequence_setup (
    PyGISequenceCache *seq_cache, GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction,
    PyGICallableCache *callable_cache);

PyGIArgCache *pygi_arg_void_type_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction);

PyGIArgCache *pygi_arg_numeric_type_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction);

PyGIArgCache *pygi_arg_string_type_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction);

PyGIArgCache *pygi_arg_enum_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction, GIEnumInfo *iface_info);

PyGIArgCache *pygi_arg_flags_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction, GIFlagsInfo *iface_info);

PyGIArgCache *pygi_arg_struct_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction,
    GIRegisteredTypeInfo *iface_info);

PyGIArgCache *pygi_arg_interface_new_from_info (
    GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction,
    GIRegisteredTypeInfo *iface_info);

PyGIArgCache *pygi_arg_callback_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction, GICallbackInfo *iface_info,
    PyGICallableCache *callable_cache);

PyGIArgCache *pygi_arg_gobject_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction,
    GIRegisteredTypeInfo *iface_info, PyGICallableCache *callable_cache);

PyGIArgCache *pygi_arg_glist_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction,
    PyGICallableCache *callable_cache);

PyGIArgCache *pygi_arg_gslist_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction,
    PyGICallableCache *callable_cache);

PyGIArgCache *pygi_arg_hash_table_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction,
    PyGICallableCache *callable_cache);

PyGIArgCache *pygi_arg_garray_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction,
    PyGICallableCache *callable_cache, gssize arg_index, gssize *py_arg_index);

PyGIArgCache *pygi_arg_gerror_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction);


gboolean pygi_marshal_from_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, PyObject *py_arg, GIArgument *arg,
    PyGIMarshalCleanupData *cleanup_data);

PyObject *pygi_marshal_to_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg,
    PyGIMarshalCleanupData *cleanup_data);


/* Needed for hack in pygi-cache-array.c */
void pygi_arg_gvalue_from_py_cleanup (PyGIInvokeState *state,
                                      PyGIArgCache *arg_cache,
                                      PyObject *py_arg,
                                      PyGIMarshalCleanupData data,
                                      gboolean was_processed);

void pygi_marshal_cleanup_data_destroy (PyGIMarshalCleanupData *cleanup_data);

G_END_DECLS

#endif /*__PYGI_CACHE_PRIVATE_H__*/
