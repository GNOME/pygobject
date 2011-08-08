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

#include "pygi-info.h"
#include "pygi-cache.h"
#include "pygi-marshal-in.h"
#include "pygi-marshal-out.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-type.h"
#include <girepository.h>

PyGIArgCache * _arg_cache_new (GITypeInfo *type_info,
                               PyGICallableCache *callable_cache,
                               GIArgInfo *arg_info,
                               GITransfer transfer,
                               GIDirection direction,
                               gssize c_arg_index,
                               gssize py_arg_index);

PyGIArgCache * _arg_cache_new_for_interface (GIInterfaceInfo *iface_info,
                                             PyGICallableCache *callable_cache,
                                             GIArgInfo *arg_info,
                                             GITransfer transfer,
                                             GIDirection direction,
                                             gssize c_arg_index,
                                             gssize py_arg_index);
/* cleanup */
void
_pygi_arg_cache_free (PyGIArgCache *cache)
{
    if (cache == NULL)
        return;

    if (cache->type_info != NULL)
        g_base_info_unref ( (GIBaseInfo *)cache->type_info);
    if (cache->destroy_notify)
        cache->destroy_notify (cache);
    else
        g_slice_free (PyGIArgCache, cache);
}

static void
_interface_cache_free_func (PyGIInterfaceCache *cache)
{
    if (cache != NULL) {
        Py_XDECREF (cache->py_type);
        if (cache->type_name != NULL)
            g_free (cache->type_name);
        if (cache->interface_info != NULL)
            g_base_info_unref ( (GIBaseInfo *)cache->interface_info);
        g_slice_free (PyGIInterfaceCache, cache);
    }
}

static void
_hash_cache_free_func (PyGIHashCache *cache)
{
    if (cache != NULL) {
        _pygi_arg_cache_free (cache->key_cache);
        _pygi_arg_cache_free (cache->value_cache);
        g_slice_free (PyGIHashCache, cache);
    }
}

static void
_sequence_cache_free_func (PyGISequenceCache *cache)
{
    if (cache != NULL) {
        _pygi_arg_cache_free (cache->item_cache);
        g_slice_free (PyGISequenceCache, cache);
    }
}

static void
_callback_cache_free_func (PyGICallbackCache *cache)
{
    if (cache != NULL) {
        if (cache->interface_info != NULL)
            g_base_info_unref ( (GIBaseInfo *)cache->interface_info);

        g_slice_free (PyGICallbackCache, cache);
    }
}

void
_pygi_callable_cache_free (PyGICallableCache *cache)
{
    gssize i;

    if (cache == NULL)
        return;

    g_slist_free (cache->out_args);
    g_slist_free (cache->arg_name_list);
    g_hash_table_destroy (cache->arg_name_hash);

    for (i = 0; i < cache->n_args; i++) {
        PyGIArgCache *tmp = cache->args_cache[i];
        _pygi_arg_cache_free (tmp);
    }
    if (cache->return_cache != NULL)
        _pygi_arg_cache_free (cache->return_cache);

    g_slice_free1 (cache->n_args * sizeof (PyGIArgCache *), cache->args_cache);
    g_slice_free (PyGICallableCache, cache);
}

/* cache generation */

static PyGIInterfaceCache *
_interface_cache_new (GIInterfaceInfo *iface_info)
{
    PyGIInterfaceCache *ic;

    ic = g_slice_new0 (PyGIInterfaceCache);
    ( (PyGIArgCache *)ic)->destroy_notify = (GDestroyNotify)_interface_cache_free_func;
    ic->g_type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *)iface_info);
    ic->py_type = _pygi_type_import_by_gi_info ( (GIBaseInfo *) iface_info);

    if (ic->py_type == NULL)
        return NULL;

    ic->type_name = _pygi_g_base_info_get_fullname (iface_info);
    return ic;
}

static PyGISequenceCache *
_sequence_cache_new (GITypeInfo *type_info,
                     GIDirection direction,
                     GITransfer transfer,
                     gssize child_offset)
{
    PyGISequenceCache *sc;
    GITypeInfo *item_type_info;
    GITypeTag item_type_tag;
    GITransfer item_transfer;

    sc = g_slice_new0 (PyGISequenceCache);
    ( (PyGIArgCache *)sc)->destroy_notify = (GDestroyNotify)_sequence_cache_free_func;

    sc->fixed_size = -1;
    sc->len_arg_index = -1;
    sc->is_zero_terminated = g_type_info_is_zero_terminated (type_info);
    if (!sc->is_zero_terminated) {
        sc->fixed_size = g_type_info_get_array_fixed_size (type_info);
        if (sc->fixed_size < 0)
            sc->len_arg_index = g_type_info_get_array_length (type_info) + child_offset;
    }

    item_type_info = g_type_info_get_param_type (type_info, 0);

    item_transfer =
        transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

    sc->item_cache = _arg_cache_new (item_type_info,
                                     NULL,
                                     NULL,
                                     item_transfer,
                                     direction,
                                     0, 0);

    if (sc->item_cache == NULL) {
        _pygi_arg_cache_free ( (PyGIArgCache *)sc);
        return NULL;
    }

    sc->item_size = _pygi_g_type_info_size (item_type_info);
    g_base_info_unref ( (GIBaseInfo *)item_type_info);

    return sc;
}
static PyGIHashCache *
_hash_cache_new (GITypeInfo *type_info,
                 GIDirection direction,
                 GITransfer transfer)
{
    PyGIHashCache *hc;
    GITypeInfo *key_type_info;
    GITypeInfo *value_type_info;
    GITransfer item_transfer;

    hc = g_slice_new0 (PyGIHashCache);
    ( (PyGIArgCache *)hc)->destroy_notify = (GDestroyNotify)_hash_cache_free_func;
    key_type_info = g_type_info_get_param_type (type_info, 0);
    value_type_info = g_type_info_get_param_type (type_info, 1);

    item_transfer =
        transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

    hc->key_cache = _arg_cache_new (key_type_info,
                                    NULL,
                                    NULL,
                                    item_transfer,
                                    direction,
                                    0, 0);

    if (hc->key_cache == NULL) {
        _pygi_arg_cache_free ( (PyGIArgCache *)hc);
        return NULL;
    }

    hc->value_cache = _arg_cache_new (value_type_info,
                                      NULL,
                                      NULL,
                                      item_transfer,
                                      direction,
                                      0, 0);

    if (hc->value_cache == NULL) {
        _pygi_arg_cache_free ( (PyGIArgCache *)hc);
        return NULL;
    }

    g_base_info_unref( (GIBaseInfo *)key_type_info);
    g_base_info_unref( (GIBaseInfo *)value_type_info);

    return hc;
}

static PyGICallbackCache *
_callback_cache_new (GIArgInfo *arg_info,
                     GIInterfaceInfo *iface_info,
                     gssize child_offset)
{
   PyGICallbackCache *cc;

   cc = g_slice_new0 (PyGICallbackCache);
   cc->user_data_index = g_arg_info_get_closure (arg_info);
   if (cc->user_data_index != -1)
       cc->user_data_index += child_offset;
   cc->destroy_notify_index = g_arg_info_get_destroy (arg_info);
   if (cc->destroy_notify_index != -1)
       cc->destroy_notify_index += child_offset;
   cc->scope = g_arg_info_get_scope (arg_info);
   g_base_info_ref( (GIBaseInfo *)iface_info);
   cc->interface_info = iface_info;
   return cc;
}

static PyGIArgCache *
_arg_cache_alloc (void)
{
    return g_slice_new0 (PyGIArgCache);
}

static void
_arg_cache_in_void_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_void;
}

static void
_arg_cache_out_void_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_void;
}

static void
_arg_cache_in_boolean_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_boolean;
}

static void
_arg_cache_out_boolean_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_boolean;
}

static void
_arg_cache_in_int8_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int8;
}

static void
_arg_cache_out_int8_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int8;
}

static void
_arg_cache_in_uint8_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint8;
}

static void
_arg_cache_out_uint8_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint8;
}

static void
_arg_cache_in_int16_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int16;
}

static void
_arg_cache_out_int16_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int16;
}

static void
_arg_cache_in_uint16_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint16;
}

static void
_arg_cache_out_uint16_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint16;
}

static void
_arg_cache_in_int32_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int32;
}

static void
_arg_cache_out_int32_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int32;
}

static void
_arg_cache_in_uint32_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint32;
}

static void
_arg_cache_out_uint32_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint32;
}

static void
_arg_cache_in_int64_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int64;
}

static void
_arg_cache_out_int64_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int64;
}

static void
_arg_cache_in_uint64_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint64;
}

static void
_arg_cache_out_uint64_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint64;
}

static void
_arg_cache_in_float_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_float;
}

static void
_arg_cache_out_float_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_float;
}

static void
_arg_cache_in_double_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_double;
}

static void
_arg_cache_out_double_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_double;
}

static void
_arg_cache_in_unichar_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_unichar;
}

static void
_arg_cache_out_unichar_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_unichar;
}

static void
_arg_cache_in_gtype_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_gtype;
}

static void
_arg_cache_out_gtype_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_gtype;
}

static void
_arg_cache_in_utf8_setup (PyGIArgCache *arg_cache,
                          GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_utf8;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_utf8;
}

static void
_arg_cache_out_utf8_setup (PyGIArgCache *arg_cache,
                           GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_utf8;
    arg_cache->out_cleanup = _pygi_marshal_cleanup_out_utf8;
}

static void
_arg_cache_in_filename_setup (PyGIArgCache *arg_cache,
                              GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_filename;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_utf8;
}

static void
_arg_cache_out_filename_setup (PyGIArgCache *arg_cache,
                               GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_filename;
    arg_cache->out_cleanup = _pygi_marshal_cleanup_out_utf8;
}

static gboolean
_arg_cache_in_array_setup (PyGIArgCache *arg_cache,
                           PyGICallableCache *callable_cache,
                           GITypeInfo *type_info,
                           GITransfer transfer,
                           GIDirection direction)
{
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    seq_cache->array_type = g_type_info_get_array_type (type_info);

    arg_cache->in_marshaller = _pygi_marshal_in_array;

    if (seq_cache->len_arg_index >= 0 &&
        direction == GI_DIRECTION_IN) {
        PyGIArgCache *child_cache = 
            callable_cache->args_cache[seq_cache->len_arg_index];

        if (child_cache == NULL) {
            child_cache = _arg_cache_alloc ();
        } else if (child_cache->meta_type == PYGI_META_ARG_TYPE_CHILD) {
            return TRUE;
        }

        child_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        child_cache->direction = direction;
        child_cache->in_marshaller = NULL;
        child_cache->out_marshaller = NULL;

        callable_cache->args_cache[seq_cache->len_arg_index] = child_cache;
    }

    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_array;

    return TRUE;
}

static gboolean
_arg_cache_out_array_setup (PyGIArgCache *arg_cache,
                            PyGICallableCache *callable_cache,
                            GITypeInfo *type_info,
                            GITransfer transfer,
                            GIDirection direction,
                            gssize arg_index)
{
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    arg_cache->out_marshaller = _pygi_marshal_out_array;
    arg_cache->out_cleanup = _pygi_marshal_cleanup_out_array;

    seq_cache->array_type = g_type_info_get_array_type (type_info);

    if (seq_cache->len_arg_index >= 0) {
        PyGIArgCache *child_cache = callable_cache->args_cache[seq_cache->len_arg_index];
        if (seq_cache->len_arg_index < arg_index)
             callable_cache->n_out_child_args++;

        if (child_cache != NULL) {
            if (child_cache->meta_type == PYGI_META_ARG_TYPE_CHILD)
                return TRUE;

            callable_cache->out_args = 
                g_slist_remove (callable_cache->out_args, child_cache);
        } else {
            child_cache = _arg_cache_alloc ();
        }

        child_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        child_cache->direction = direction;
        child_cache->in_marshaller = NULL;
        child_cache->out_marshaller = NULL;

        callable_cache->args_cache[seq_cache->len_arg_index] = child_cache;
    }

    return TRUE;
}

static void
_arg_cache_in_glist_setup (PyGIArgCache *arg_cache,
                           GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_glist;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_glist;
}

static void
_arg_cache_out_glist_setup (PyGIArgCache *arg_cache,
                            GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_glist;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_out_glist;
}

static void
_arg_cache_in_gslist_setup (PyGIArgCache *arg_cache,
                            GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_gslist;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_glist;
}

static void
_arg_cache_out_gslist_setup (PyGIArgCache *arg_cache,
                             GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_gslist;
    arg_cache->out_cleanup = _pygi_marshal_cleanup_out_glist;
}

static void
_arg_cache_in_ghash_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_ghash;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_ghash;
}

static void
_arg_cache_out_ghash_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_ghash;
    arg_cache->out_cleanup = _pygi_marshal_cleanup_out_ghash;
}

static void
_arg_cache_in_gerror_setup (PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_gerror;
    arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
}

static void
_arg_cache_out_gerror_setup (PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_gerror;
    arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
}

static void
_arg_cache_in_interface_union_setup (PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;
}

static void
_arg_cache_out_interface_union_setup (PyGIArgCache *arg_cache,
                                      GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_struct;
}

static void
_arg_cache_in_interface_struct_setup (PyGIArgCache *arg_cache,
                                      GIInterfaceInfo *iface_info,
                                      GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign ( (GIStructInfo*)iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;

    if (iface_cache->g_type == G_TYPE_VALUE)
        arg_cache->in_cleanup = _pygi_marshal_cleanup_in_interface_struct_gvalue;
    else if (iface_cache->is_foreign)
        arg_cache->in_cleanup = _pygi_marshal_cleanup_in_interface_struct_foreign;
}

static void
_arg_cache_out_interface_struct_setup (PyGIArgCache *arg_cache,
                                       GIInterfaceInfo *iface_info,
                                       GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign ( (GIStructInfo*)iface_info);
    arg_cache->out_marshaller = _pygi_marshal_out_interface_struct;

    if (iface_cache->is_foreign)
        arg_cache->in_cleanup = _pygi_marshal_cleanup_out_interface_struct_foreign;
}

static void
_arg_cache_in_interface_object_setup (PyGIArgCache *arg_cache,
                                      GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_object;
    arg_cache->in_cleanup = _pygi_marshal_cleanup_in_interface_object;
}

static void
_arg_cache_out_interface_object_setup (PyGIArgCache *arg_cache,
                                       GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_object;
    arg_cache->out_cleanup = _pygi_marshal_cleanup_out_interface_object;
}

static void
_arg_cache_in_interface_callback_setup (PyGIArgCache *arg_cache,
                                        PyGICallableCache *callable_cache)
{
    PyGICallbackCache *callback_cache = (PyGICallbackCache *)arg_cache;
    if (callback_cache->user_data_index >= 0) {
        PyGIArgCache *user_data_arg_cache = _arg_cache_alloc ();
        user_data_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD_WITH_PYARG;
        callable_cache->args_cache[callback_cache->user_data_index] = user_data_arg_cache;
    }

    if (callback_cache->destroy_notify_index >= 0) {
        PyGIArgCache *destroy_arg_cache = _arg_cache_alloc ();
        destroy_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        callable_cache->args_cache[callback_cache->destroy_notify_index] = destroy_arg_cache;
    }
    arg_cache->in_marshaller = _pygi_marshal_in_interface_callback;
}

static void
_arg_cache_out_interface_callback_setup (void)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Callback returns are not supported");
}

static void
_arg_cache_in_interface_enum_setup (PyGIArgCache *arg_cache,
                                    GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_enum;
}

static void
_arg_cache_out_interface_enum_setup (PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_enum;
}

static void
_arg_cache_in_interface_flags_setup (PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_flags;
}

static void
_arg_cache_out_interface_flags_setup (PyGIArgCache *arg_cache,
                                      GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_flags;
}

PyGIArgCache *
_arg_cache_new_for_interface (GIInterfaceInfo *iface_info,
                              PyGICallableCache *callable_cache,
                              GIArgInfo *arg_info,
                              GITransfer transfer,
                              GIDirection direction,
                              gssize c_arg_index,
                              gssize py_arg_index)
{
    PyGIInterfaceCache *iface_cache = NULL;
    PyGIArgCache *arg_cache = NULL;
    gssize child_offset = 0;
    GIInfoType info_type;

    if (callable_cache != NULL)
        child_offset =
            (callable_cache->function_type == PYGI_FUNCTION_TYPE_METHOD ||
                 callable_cache->function_type == PYGI_FUNCTION_TYPE_VFUNC) ? 1: 0;

    GI_IS_INTERFACE_INFO (iface_info);

    info_type = g_base_info_get_type ( (GIBaseInfo *)iface_info);

    /* Callbacks are special cased */
    if (info_type != GI_INFO_TYPE_CALLBACK) {
        iface_cache = _interface_cache_new (iface_info);

        arg_cache = (PyGIArgCache *)iface_cache;
        if (arg_cache == NULL)
            return NULL;
    }

    switch (info_type) {
        case GI_INFO_TYPE_UNION:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_union_setup (arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_union_setup (arg_cache, transfer);

            break;
        case GI_INFO_TYPE_STRUCT:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_struct_setup (arg_cache,
                                                     iface_info,
                                                     transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_struct_setup (arg_cache,
                                                      iface_info,
                                                      transfer);

            break;
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_object_setup (arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_object_setup (arg_cache, transfer);

            break;
        case GI_INFO_TYPE_BOXED:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                _arg_cache_in_interface_struct_setup (arg_cache,
                                                      iface_info,
                                                      transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                _arg_cache_out_interface_struct_setup (arg_cache,
                                                       iface_info,
                                                       transfer);

            break;
        case GI_INFO_TYPE_CALLBACK:
            {
                PyGICallbackCache *callback_cache;

                if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                    _arg_cache_out_interface_callback_setup ();
                    return NULL;
                }

                callback_cache =
                    _callback_cache_new (arg_info,
                                         iface_info,
                                         child_offset);

                arg_cache = (PyGIArgCache *)callback_cache;
                if (arg_cache == NULL)
                    return NULL;

                if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                    _arg_cache_in_interface_callback_setup (arg_cache, callable_cache);

                break;
            }
        case GI_INFO_TYPE_ENUM:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_enum_setup (arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_enum_setup (arg_cache, transfer);

            break;
        case GI_INFO_TYPE_FLAGS:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_flags_setup (arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_flags_setup (arg_cache, transfer);

            break;
        default:
            g_assert_not_reached ();
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = GI_TYPE_TAG_INTERFACE;
        arg_cache->py_arg_index = py_arg_index;
        arg_cache->c_arg_index = c_arg_index;

        if (iface_cache != NULL) {
            g_base_info_ref ( (GIBaseInfo *)iface_info);
            iface_cache->interface_info = iface_info;
        }
    }

    return arg_cache;
}

PyGIArgCache *
_arg_cache_new (GITypeInfo *type_info,
                PyGICallableCache *callable_cache,
                GIArgInfo *arg_info,
                GITransfer transfer,
                GIDirection direction,
                gssize c_arg_index,
                gssize py_arg_index)
{
    PyGIArgCache *arg_cache = NULL;
    gssize child_offset = 0;
    GITypeTag type_tag;
    
    GI_IS_TYPE_INFO (type_info);

    type_tag = g_type_info_get_tag (type_info);

    if (callable_cache != NULL)
        child_offset =
            (callable_cache->function_type == PYGI_FUNCTION_TYPE_METHOD ||
                callable_cache->function_type == PYGI_FUNCTION_TYPE_VFUNC) ? 1: 0;

    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_void_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_void_setup (arg_cache);
           break;
       case GI_TYPE_TAG_BOOLEAN:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_boolean_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_boolean_setup (arg_cache);

           break;
       case GI_TYPE_TAG_INT8:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int8_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int8_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UINT8:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint8_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint8_setup (arg_cache);

           break;
       case GI_TYPE_TAG_INT16:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int16_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int16_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UINT16:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint16_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint16_setup (arg_cache);

           break;
       case GI_TYPE_TAG_INT32:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int32_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int32_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UINT32:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint32_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint32_setup (arg_cache);

           break;
       case GI_TYPE_TAG_INT64:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int64_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int64_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UINT64:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint64_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint64_setup (arg_cache);

           break;
       case GI_TYPE_TAG_FLOAT:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_float_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_float_setup (arg_cache);

           break;
       case GI_TYPE_TAG_DOUBLE:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_double_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_double_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UNICHAR:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_unichar_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_unichar_setup (arg_cache);

           break;
       case GI_TYPE_TAG_GTYPE:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_gtype_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_gtype_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UTF8:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_utf8_setup (arg_cache, transfer);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_utf8_setup (arg_cache, transfer);

           break;
       case GI_TYPE_TAG_FILENAME:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_filename_setup (arg_cache, transfer);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_filename_setup (arg_cache, transfer);

           break;
       case GI_TYPE_TAG_ARRAY:
           {
               PyGISequenceCache *seq_cache =
                   _sequence_cache_new (type_info,
                                        direction,
                                        transfer,
                                        child_offset);

               arg_cache = (PyGIArgCache *)seq_cache;
               if (arg_cache == NULL)
                   break;

               if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                   _arg_cache_in_array_setup (arg_cache,
                                              callable_cache,
                                              type_info,
                                              transfer,
                                              direction);

               if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                   _arg_cache_out_array_setup (arg_cache,
                                               callable_cache,
                                               type_info,
                                               transfer,
                                               direction,
                                               c_arg_index);

               /* ugly edge case code:
                *  
                * length can come before the array parameter which means we
                * need to update indexes if this happens
                */ 
               if (seq_cache->len_arg_index > -1 &&
                   seq_cache->len_arg_index < c_arg_index) {
                   gssize i;

                   py_arg_index -= 1;
                   callable_cache->n_py_args -= 1;

                   for (i = seq_cache->len_arg_index + 1; 
                          i < callable_cache->n_args; 
                            i++) {
                       PyGIArgCache *update_cache = callable_cache->args_cache[i];
                       if (update_cache == NULL)
                           break;

                       update_cache->py_arg_index -= 1;
                   }
               }

               break;
           }
       case GI_TYPE_TAG_GLIST:
           {
               PyGISequenceCache *seq_cache =
                   _sequence_cache_new (type_info,
                                        direction,
                                        transfer,
                                        child_offset);

               arg_cache = (PyGIArgCache *)seq_cache;
               if (arg_cache == NULL)
                   break;

               if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                   _arg_cache_in_glist_setup (arg_cache, transfer);

               if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                   _arg_cache_out_glist_setup (arg_cache, transfer);


               break;
           }
       case GI_TYPE_TAG_GSLIST:
           {
               PyGISequenceCache *seq_cache =
                   _sequence_cache_new (type_info,
                                        direction,
                                        transfer,
                                        child_offset);

               arg_cache = (PyGIArgCache *)seq_cache;
               if (arg_cache == NULL)
                   break;

               if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                   _arg_cache_in_gslist_setup (arg_cache, transfer);

               if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                   _arg_cache_out_gslist_setup (arg_cache, transfer);

               break;
            }
       case GI_TYPE_TAG_GHASH:
           arg_cache =
               (PyGIArgCache *)_hash_cache_new (type_info,
                                                direction,
                                                transfer);

           if (arg_cache == NULL)
                   break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_ghash_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
               _arg_cache_out_ghash_setup (arg_cache);
           }

           break;
       case GI_TYPE_TAG_INTERFACE:
           {
               GIInterfaceInfo *interface_info = g_type_info_get_interface (type_info);
               arg_cache = _arg_cache_new_for_interface (interface_info,
                                                         callable_cache,
                                                         arg_info,
                                                         transfer,
                                                         direction,
                                                         c_arg_index,
                                                         py_arg_index);

               g_base_info_unref ( (GIBaseInfo *)interface_info);
               break;
           }
       case GI_TYPE_TAG_ERROR:
           arg_cache = _arg_cache_alloc ();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_gerror_setup (arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_gerror_setup (arg_cache);

           break;
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = type_tag;
        arg_cache->py_arg_index = py_arg_index;
        arg_cache->c_arg_index = c_arg_index;
        arg_cache->is_pointer = g_type_info_is_pointer (type_info);
        g_base_info_ref ( (GIBaseInfo *) type_info);
        arg_cache->type_info = type_info;
    }

    return arg_cache;
}

static void
_arg_name_list_generate (PyGICallableCache *callable_cache)
{
    GSList * arg_name_list = NULL;

    if (callable_cache->arg_name_hash == NULL) {
        callable_cache->arg_name_hash = g_hash_table_new (g_str_hash, g_str_equal);
    } else {
        g_hash_table_remove_all (callable_cache->arg_name_hash);
    }

    for (int i=0; i < callable_cache->n_args; i++) {
        PyGIArgCache *arg_cache = NULL;

        arg_cache = callable_cache->args_cache[i];

        if (arg_cache->meta_type != PYGI_META_ARG_TYPE_CHILD &&
                (arg_cache->direction == GI_DIRECTION_IN ||
                 arg_cache->direction == GI_DIRECTION_INOUT)) {

            gpointer arg_name = (gpointer)arg_cache->arg_name;

            arg_name_list = g_slist_prepend (arg_name_list, arg_name);
            if (arg_name != NULL) {
                g_hash_table_insert (callable_cache->arg_name_hash, arg_name, arg_name);
            }
        }
    }

    callable_cache->arg_name_list = g_slist_reverse (arg_name_list);
}

/* Generate the cache for the callable's arguments */
static gboolean
_args_cache_generate (GICallableInfo *callable_info,
                      PyGICallableCache *callable_cache)
{
    gssize arg_index = 0;
    gssize i;
    GITypeInfo *return_info;
    GITransfer return_transfer;
    PyGIArgCache *return_cache;

    /* cache the return arg */
    return_info =
        g_callable_info_get_return_type (callable_info);
    return_transfer =
        g_callable_info_get_caller_owns (callable_info);
    return_cache =
        _arg_cache_new (return_info,
                        callable_cache,
                        NULL,
                        return_transfer,
                        GI_DIRECTION_OUT,
                        -1,
                        -1);

    callable_cache->return_cache = return_cache;
    g_base_info_unref (return_info);

    /* first arg is the instance */
    if (callable_cache->function_type == PYGI_FUNCTION_TYPE_METHOD ||
            callable_cache->function_type == PYGI_FUNCTION_TYPE_VFUNC) {
        GIInterfaceInfo *interface_info;
        PyGIArgCache *instance_cache;

        interface_info = g_base_info_get_container ( (GIBaseInfo *)callable_info);

        instance_cache =
            _arg_cache_new_for_interface (interface_info,
                                          callable_cache,
                                          NULL,
                                          GI_TRANSFER_NOTHING,
                                          GI_DIRECTION_IN,
                                          arg_index,
                                          0);

        instance_cache->in_marshaller = _pygi_marshal_in_interface_instance;
        g_base_info_unref ( (GIBaseInfo *)interface_info);

        if (instance_cache == NULL)
            return FALSE;

        callable_cache->args_cache[arg_index] = instance_cache;

        arg_index++;
        callable_cache->n_in_args++;
        callable_cache->n_py_args++;
    }


    for (i=0; arg_index < callable_cache->n_args; arg_index++, i++) {
        PyGIArgCache *arg_cache = NULL;
        GIArgInfo *arg_info;
        GITypeInfo *type_info;
        GIDirection direction;
        GITransfer transfer;
        GITypeTag type_tag;
        gboolean is_caller_allocates = FALSE;
        gssize py_arg_index = -1;

        arg_info =
            g_callable_info_get_arg (callable_info, i);

        direction = g_arg_info_get_direction (arg_info);
        transfer = g_arg_info_get_ownership_transfer (arg_info);
        type_info = g_arg_info_get_type (arg_info);
        type_tag = g_type_info_get_tag (type_info);

        if (type_tag == GI_TYPE_TAG_INTERFACE)
            is_caller_allocates = g_arg_info_is_caller_allocates (arg_info);

        /* must be an child arg filled in by its owner
         * fill in it's c_arg_index, add to the in count
         * and continue
         */
        if (callable_cache->args_cache[arg_index] != NULL) {
            arg_cache = callable_cache->args_cache[arg_index];
            if (arg_cache->meta_type == PYGI_META_ARG_TYPE_CHILD_WITH_PYARG) {
                arg_cache->py_arg_index = callable_cache->n_py_args;
                callable_cache->n_py_args++;
            }

            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                arg_cache->c_arg_index = callable_cache->n_in_args;
                callable_cache->n_in_args++;
            }

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                callable_cache->n_out_args++;
                callable_cache->n_out_child_args++;
            }

            g_base_info_unref ( (GIBaseInfo *)arg_info);
            continue;
        }

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            py_arg_index = callable_cache->n_py_args;
            callable_cache->n_in_args++;
            callable_cache->n_py_args++;
        }

        arg_cache =
            _arg_cache_new (type_info,
                            callable_cache,
                            arg_info,
                            transfer,
                            direction,
                            arg_index,
                            py_arg_index);

        if (arg_cache == NULL)
            goto arg_err;

        arg_cache->arg_name = g_base_info_get_name ((GIBaseInfo *) arg_info);
        arg_cache->allow_none = g_arg_info_may_be_null(arg_info);
        arg_cache->is_caller_allocates = is_caller_allocates;

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            callable_cache->n_out_args++;

            if (arg_cache == NULL)
                goto arg_err;

            callable_cache->out_args =
                g_slist_append (callable_cache->out_args, arg_cache);
        }

        callable_cache->args_cache[arg_index] = arg_cache;
        g_base_info_unref( (GIBaseInfo *)type_info);
        g_base_info_unref( (GIBaseInfo *)arg_info);

        continue;
arg_err:
        g_base_info_unref( (GIBaseInfo *)type_info);
        g_base_info_unref( (GIBaseInfo *)arg_info);
        return FALSE;
    }

    _arg_name_list_generate (callable_cache);

    return TRUE;
}

PyGICallableCache *
_pygi_callable_cache_new (GICallableInfo *callable_info)
{
    PyGICallableCache *cache;
    GIInfoType type = g_base_info_get_type ( (GIBaseInfo *)callable_info);

    cache = g_slice_new0 (PyGICallableCache);

    if (cache == NULL)
        return NULL;

    cache->name = g_base_info_get_name ((GIBaseInfo *)callable_info);

    if (type == GI_INFO_TYPE_FUNCTION) {
        GIFunctionInfoFlags flags;

        flags = g_function_info_get_flags ( (GIFunctionInfo *)callable_info);

        if (flags & GI_FUNCTION_IS_CONSTRUCTOR)
            cache->function_type = PYGI_FUNCTION_TYPE_CONSTRUCTOR;
        else if (flags & GI_FUNCTION_IS_METHOD)
            cache->function_type = PYGI_FUNCTION_TYPE_METHOD;
    } else if (type == GI_INFO_TYPE_VFUNC) {
        cache->function_type = PYGI_FUNCTION_TYPE_VFUNC;
    } else if (type == GI_INFO_TYPE_CALLBACK) {
        cache->function_type = PYGI_FUNCTION_TYPE_CALLBACK;
    } else {
        cache->function_type = PYGI_FUNCTION_TYPE_METHOD;
    }

    cache->n_args = g_callable_info_get_n_args (callable_info);

    /* if we are a method or vfunc make sure the instance parameter is counted */
    if (cache->function_type == PYGI_FUNCTION_TYPE_METHOD ||
            cache->function_type == PYGI_FUNCTION_TYPE_VFUNC)
        cache->n_args++;

    if (cache->n_args > 0)
        cache->args_cache = g_slice_alloc0 (cache->n_args * sizeof (PyGIArgCache *));

    if (!_args_cache_generate (callable_info, cache))
        goto err;

    return cache;
err:
    _pygi_callable_cache_free (cache);
    return NULL;
}
