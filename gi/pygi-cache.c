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
#include "pygi-argument.h"
#include "pygi-type.h"
#include <girepository.h>

PyGIArgCache * _arg_cache_new_from_type_info (GITypeInfo *type_info,
                                              PyGIFunctionCache *function_cache,
                                              GIArgInfo *arg_info,
                                              GITypeTag type_tag,
                                              GITransfer transfer,
                                              GIDirection direction,
                                              gboolean is_caller_allocates,
                                              gint c_arg_index,
                                              gint py_arg_index);

/* cleanup */
void
_pygi_arg_cache_free(PyGIArgCache *cache)
{
    if (cache == NULL)
        return;

    if (cache->type_info != NULL)
        g_base_info_unref( (GIBaseInfo *)cache->type_info);
    if (cache->destroy_notify)
        cache->destroy_notify(cache);
    else
        g_slice_free(PyGIArgCache, cache);
}

static void
_interface_cache_free_func (PyGIInterfaceCache *cache)
{
    if (cache != NULL) {
        Py_XDECREF(cache->py_type);
        if (cache->type_name != NULL)
            g_free(cache->type_name);
        if (cache->interface_info != NULL)
            g_base_info_unref( (GIBaseInfo *)cache->interface_info);
        g_slice_free(PyGIInterfaceCache, cache);
    }
}

static void
_hash_cache_free_func(PyGIHashCache *cache)
{
    if (cache != NULL) {
        _pygi_arg_cache_free(cache->key_cache);
        _pygi_arg_cache_free(cache->value_cache);
        g_slice_free(PyGIHashCache, cache);
    }
}

static void
_sequence_cache_free_func(PyGISequenceCache *cache)
{
    if (cache != NULL) {
        _pygi_arg_cache_free(cache->item_cache);
        g_slice_free(PyGISequenceCache, cache);
    }
}

static void
_callback_cache_free_func(PyGICallbackCache *cache)
{
    if (cache != NULL) {
        if (cache->interface_info != NULL)
            g_base_info_unref( (GIBaseInfo *)cache->interface_info);

        g_slice_free(PyGICallbackCache, cache);
    }
}

void
_pygi_function_cache_free(PyGIFunctionCache *cache)
{
    int i;

    if (cache == NULL)
        return;

    g_slist_free(cache->out_args);
    for (i = 0; i < cache->n_args; i++) {
        PyGIArgCache *tmp = cache->args_cache[i];
        _pygi_arg_cache_free(tmp);
    }
    if (cache->return_cache != NULL)
        _pygi_arg_cache_free(cache->return_cache);

    g_slice_free1(cache->n_args * sizeof(PyGIArgCache *), cache->args_cache);
    g_slice_free(PyGIFunctionCache, cache);
}

/* cache generation */
static inline PyGIFunctionCache *
_function_cache_new_from_function_info(GIFunctionInfo *function_info)
{
    PyGIFunctionCache *fc;
    GIFunctionInfoFlags flags;

    fc = g_slice_new0(PyGIFunctionCache);

    fc->name = g_base_info_get_name((GIBaseInfo *)function_info);
    flags = g_function_info_get_flags(function_info);
    fc->is_method = flags & GI_FUNCTION_IS_METHOD;
    fc->is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;

    fc->n_args = g_callable_info_get_n_args ( (GICallableInfo *) function_info) + (fc->is_method ? 1: 0);
    if (fc->n_args > 0)
        fc->args_cache = g_slice_alloc0(fc->n_args * sizeof(PyGIArgCache *));

    return fc;
}

static inline PyGIInterfaceCache *
_interface_cache_new_from_interface_info(GIInterfaceInfo *iface_info)
{
    PyGIInterfaceCache *ic;

    ic = g_slice_new0(PyGIInterfaceCache);
    ((PyGIArgCache *)ic)->destroy_notify = (GDestroyNotify)_interface_cache_free_func;
    ic->g_type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *)iface_info);
    if (ic->g_type != G_TYPE_NONE) {
        ic->py_type = _pygi_type_get_from_g_type (ic->g_type);
    } else {
        /* FIXME: avoid passing GI infos to noncache API */
        ic->py_type = _pygi_type_import_by_gi_info ( (GIBaseInfo *) iface_info);
    }

    if (ic->py_type == NULL)
        return NULL;

    ic->type_name = _pygi_g_base_info_get_fullname(iface_info);
    return ic;
}

static inline PyGISequenceCache *
_sequence_cache_new_from_type_info(GITypeInfo *type_info,
                                   GIDirection direction,
                                   GITransfer transfer,
                                   gint aux_offset)
{
    PyGISequenceCache *sc;
    GITypeInfo *item_type_info;
    GITypeTag item_type_tag;
    GITransfer item_transfer;

    sc = g_slice_new0(PyGISequenceCache);
    ((PyGIArgCache *)sc)->destroy_notify = (GDestroyNotify)_sequence_cache_free_func;

    sc->fixed_size = -1;
    sc->len_arg_index = -1;
    sc->is_zero_terminated = g_type_info_is_zero_terminated(type_info);
    if (!sc->is_zero_terminated)
        sc->fixed_size = g_type_info_get_array_fixed_size(type_info);
    if (sc->fixed_size < 0)
        sc->len_arg_index = g_type_info_get_array_length (type_info) + aux_offset;

    item_type_info = g_type_info_get_param_type (type_info, 0);
    item_type_tag = g_type_info_get_tag (item_type_info);

    item_transfer = GI_TRANSFER_NOTHING;
    if (transfer == GI_TRANSFER_EVERYTHING ||
        transfer == GI_TRANSFER_CONTAINER)
        item_transfer = GI_TRANSFER_EVERYTHING;

    sc->item_cache = _arg_cache_new_from_type_info(item_type_info,
                                                   NULL,
                                                   NULL,
                                                   item_type_tag,
                                                   item_transfer,
                                                   direction,
                                                   FALSE,
                                                   0, 0);

    if (sc->item_cache == NULL) {
        _pygi_arg_cache_free((PyGIArgCache *)sc);
        return NULL;
    }

    sc->item_cache->type_tag = item_type_tag;
    sc->item_size = _pygi_g_type_info_size(item_type_info);
    g_base_info_unref( (GIBaseInfo *) item_type_info);

    return sc;
}
static inline PyGIHashCache *
_hash_cache_new_from_type_info(GITypeInfo *type_info,
                               GIDirection direction,
                               GITransfer transfer)
{
    PyGIHashCache *hc;
    GITypeInfo *key_type_info;
    GITypeTag key_type_tag;
    GITypeInfo *value_type_info;
    GITypeTag value_type_tag;
    GITransfer item_transfer;

    hc = g_slice_new0(PyGIHashCache);
    ((PyGIArgCache *)hc)->destroy_notify = (GDestroyNotify)_hash_cache_free_func;
    key_type_info = g_type_info_get_param_type (type_info, 0);
    key_type_tag = g_type_info_get_tag (key_type_info);
    value_type_info = g_type_info_get_param_type (type_info, 1);
    value_type_tag = g_type_info_get_tag (value_type_info);

    item_transfer = GI_TRANSFER_NOTHING;
    if (transfer == GI_TRANSFER_EVERYTHING ||
        transfer == GI_TRANSFER_CONTAINER)
        item_transfer = GI_TRANSFER_EVERYTHING;

    hc->key_cache = _arg_cache_new_from_type_info(key_type_info,
                                                  NULL,
                                                  NULL,
                                                  key_type_tag,
                                                  item_transfer,
                                                  direction,
                                                  FALSE,
                                                  0, 0);

    if (hc->key_cache == NULL) {
        _pygi_arg_cache_free((PyGIArgCache *)hc);
        return NULL;
    }

    hc->value_cache = _arg_cache_new_from_type_info(value_type_info,
                                                    NULL,
                                                    NULL,
                                                    value_type_tag,
                                                    item_transfer,
                                                    direction,
                                                    FALSE,
                                                    0, 0);

    if (hc->value_cache == NULL) {
        _pygi_arg_cache_free((PyGIArgCache *)hc);
        return NULL;
    }

    hc->key_cache->type_tag = key_type_tag;
    hc->value_cache->type_tag = value_type_tag;

    g_base_info_unref( (GIBaseInfo *)key_type_info);
    g_base_info_unref( (GIBaseInfo *)value_type_info);

    return hc;
}

static inline PyGICallbackCache *
_callback_cache_new_from_arg_info(GIArgInfo *arg_info,
                                  GIInterfaceInfo *iface_info,
                                  gint aux_offset)
{
   PyGICallbackCache *cc;

   cc = g_slice_new0(PyGICallbackCache);
   cc->user_data_index = g_arg_info_get_closure(arg_info) + aux_offset;
   cc->destroy_notify_index = g_arg_info_get_destroy(arg_info) + aux_offset;
   cc->scope = g_arg_info_get_scope(arg_info);
   g_base_info_ref( (GIBaseInfo *)iface_info);
   cc->interface_info = iface_info;
   return cc;
}

static inline PyGIArgCache *
_arg_cache_new(void)
{
    return g_slice_new0(PyGIArgCache);
}

static inline void
_arg_cache_in_void_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_void;
}

static inline void
_arg_cache_out_void_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_void;
}

static inline void
_arg_cache_in_boolean_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_boolean;
}

static inline void
_arg_cache_out_boolean_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_boolean;
}

static inline void
_arg_cache_in_int8_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int8;
}

static inline void
_arg_cache_out_int8_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int8;
}

static inline void
_arg_cache_in_uint8_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint8;
}

static inline void
_arg_cache_out_uint8_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint8;
}

static inline void
_arg_cache_in_int16_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int16;
}

static inline void
_arg_cache_out_int16_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int16;
}

static inline void
_arg_cache_in_uint16_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint16;
}

static inline void
_arg_cache_out_uint16_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint16;
}

static inline void
_arg_cache_in_int32_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int32;
}

static inline void
_arg_cache_out_int32_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int32;
}

static inline void
_arg_cache_in_uint32_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint32;
}

static inline void
_arg_cache_out_uint32_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint32;
}

static inline void
_arg_cache_in_int64_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_int64;
}

static inline void
_arg_cache_out_int64_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_int64;
}

static inline void
_arg_cache_in_uint64_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_uint64;
}

static inline void
_arg_cache_out_uint64_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_uint64;
}

static inline void
_arg_cache_in_float_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_float;
}

static inline void
_arg_cache_out_float_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_float;
}

static inline void
_arg_cache_in_double_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_double;
}

static inline void
_arg_cache_out_double_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_double;
}

static inline void
_arg_cache_in_unichar_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_unichar;
}

static inline void
_arg_cache_out_unichar_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_unichar;
}

static inline void
_arg_cache_in_gtype_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_gtype;
}

static inline void
_arg_cache_out_gtype_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_gtype;
}

static inline void
_arg_cache_in_utf8_setup(PyGIArgCache *arg_cache,
                         GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_utf8;
}

static inline void
_arg_cache_out_utf8_setup(PyGIArgCache *arg_cache,
                          GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_utf8;
}

static inline void
_arg_cache_in_filename_setup(PyGIArgCache *arg_cache,
                             GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_filename;
}

static inline void
_arg_cache_out_filename_setup(PyGIArgCache *arg_cache,
                              GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_filename;
}

static inline gboolean
_arg_cache_in_array_setup(PyGIArgCache *arg_cache,
                          PyGIFunctionCache *function_cache,
                          GITypeInfo *type_info,
                          GITransfer transfer,
                          GIDirection direction)
{
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    seq_cache->array_type = g_type_info_get_array_type(type_info);

    if (seq_cache->len_arg_index >= 0) {
        PyGIArgCache *aux_cache = _arg_cache_new();
        aux_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
        aux_cache->direction = direction;
        if (function_cache->args_cache[seq_cache->len_arg_index] != NULL) {
            PyGIArgCache *invalid_cache = function_cache->args_cache[seq_cache->len_arg_index];
            arg_cache->c_arg_index = invalid_cache->c_arg_index;
            _pygi_arg_cache_free(invalid_cache);
        }

        function_cache->args_cache[seq_cache->len_arg_index] = aux_cache;
    }

    arg_cache->in_marshaller = _pygi_marshal_in_array;

    /* arg_cache->cleanup = _pygi_cleanup_array; */
}

static inline gboolean
_arg_cache_out_array_setup(PyGIArgCache *arg_cache,
                           PyGIFunctionCache *function_cache,
                           GITypeInfo *type_info,
                           GITransfer transfer,
                           gssize arg_index)
{
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    arg_cache->out_marshaller = _pygi_marshal_out_array;

    seq_cache->array_type = g_type_info_get_array_type(type_info);

    if (seq_cache->len_arg_index >= 0) {
        PyGIArgCache *aux_cache = function_cache->args_cache[seq_cache->len_arg_index];
        if (seq_cache->len_arg_index < arg_index)
             function_cache->n_out_aux_args++;

        if (aux_cache == NULL) {
            aux_cache = _arg_cache_new();
        } else if (aux_cache->aux_type == PYGI_AUX_TYPE_IGNORE) {
            return TRUE;
        }

        aux_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
        if (function_cache->args_cache[seq_cache->len_arg_index] != NULL) {
            PyGIArgCache *invalid_cache = function_cache->args_cache[seq_cache->len_arg_index];
            arg_cache->c_arg_index = invalid_cache->c_arg_index;
            _pygi_arg_cache_free(invalid_cache);
        }

        if (aux_cache->direction != GI_DIRECTION_INOUT)
            aux_cache->direction = GI_DIRECTION_OUT;

        function_cache->args_cache[seq_cache->len_arg_index] = aux_cache;
    }

    return TRUE;
    /* arg_cache->cleanup = _pygi_cleanup_array; */
}

static inline void
_arg_cache_in_glist_setup(PyGIArgCache *arg_cache,
                          GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_glist;
    /* arg_cache->cleanup = */
}

static inline void
_arg_cache_out_glist_setup(PyGIArgCache *arg_cache,
                           GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_glist;
    /* arg_cache->cleanup = */
}

static inline void
_arg_cache_in_gslist_setup(PyGIArgCache *arg_cache,
                           GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_gslist;
    /* arg_cache->cleanup = */
}

static inline void
_arg_cache_out_gslist_setup(PyGIArgCache *arg_cache,
                            GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_gslist;
    /* arg_cache->cleanup = */
}

static inline void
_arg_cache_in_ghash_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_ghash;
}

static inline void
_arg_cache_out_ghash_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_ghash;
}

static inline void
_arg_cache_in_gerror_setup(PyGIArgCache *arg_cache)
{
    arg_cache->in_marshaller = _pygi_marshal_in_gerror;
    arg_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
}

static inline void
_arg_cache_out_gerror_setup(PyGIArgCache *arg_cache)
{
    arg_cache->out_marshaller = _pygi_marshal_out_gerror;
    arg_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
}

static inline void
_arg_cache_in_interface_union_setup(PyGIArgCache *arg_cache,
                                    GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;
}

static inline void
_arg_cache_out_interface_union_setup(PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_struct;
}

static void
_g_slice_free_gvalue_func(GValue *value) {
    g_slice_free(GValue, value);
}

static inline void
_arg_cache_in_interface_struct_setup(PyGIArgCache *arg_cache,
                                     GIInterfaceInfo *iface_info,
                                     GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign( (GIStructInfo*)iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;
    if (iface_cache->g_type == G_TYPE_VALUE)
        arg_cache->cleanup = _g_slice_free_gvalue_func;
    if (iface_cache->g_type == G_TYPE_CLOSURE)
        arg_cache->cleanup = g_closure_unref;
}

static inline void
_arg_cache_out_interface_struct_setup(PyGIArgCache *arg_cache,
                                      GIInterfaceInfo *iface_info,
                                      GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign( (GIStructInfo*)iface_info);
    arg_cache->out_marshaller = _pygi_marshal_out_interface_struct;
}

static inline void
_arg_cache_in_interface_object_setup(PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_object;
    if (transfer == GI_TRANSFER_EVERYTHING)
        arg_cache->cleanup = (GDestroyNotify)g_object_unref;
}

static inline void
_arg_cache_out_interface_object_setup(PyGIArgCache *arg_cache,
                                      GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_object;
}

static inline void
_arg_cache_in_interface_boxed_setup(PyGIArgCache *arg_cache,
                                    GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_boxed;
}

static inline void
_arg_cache_out_interface_boxed_setup(PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_struct;
}

static inline void
_arg_cache_in_interface_callback_setup(PyGIArgCache *arg_cache,
                                       PyGIFunctionCache *function_cache)
{
    PyGICallbackCache *callback_cache = (PyGICallbackCache *)arg_cache;
    if (callback_cache->user_data_index >= 0) {
        PyGIArgCache *user_data_arg_cache = _arg_cache_new();
        user_data_arg_cache->aux_type = PYGI_AUX_TYPE_HAS_PYARG;
        function_cache->args_cache[callback_cache->user_data_index] = user_data_arg_cache;
    }

    if (callback_cache->destroy_notify_index >= 0) {
        PyGIArgCache *destroy_arg_cache = _arg_cache_new();
        destroy_arg_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
        function_cache->args_cache[callback_cache->destroy_notify_index] = destroy_arg_cache;
    }
    arg_cache->in_marshaller = _pygi_marshal_in_interface_callback;
}

static inline void
_arg_cache_out_interface_callback_setup(void)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Callback returns are not supported");
}

static inline void
_arg_cache_in_interface_enum_setup(PyGIArgCache *arg_cache,
                                   GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_enum;
}

static inline void
_arg_cache_out_interface_enum_setup(PyGIArgCache *arg_cache,
                                    GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_enum;
}

static inline void
_arg_cache_in_interface_flags_setup(PyGIArgCache *arg_cache,
                                    GITransfer transfer)
{
    arg_cache->in_marshaller = _pygi_marshal_in_interface_flags;
}

static inline void
_arg_cache_out_interface_flags_setup(PyGIArgCache *arg_cache,
                                     GITransfer transfer)
{
    arg_cache->out_marshaller = _pygi_marshal_out_interface_flags;
}

static inline PyGIArgCache *
_arg_cache_new_from_interface_info (GIInterfaceInfo *iface_info,
                                    PyGIFunctionCache *function_cache,
                                    GIArgInfo *arg_info,
                                    GIInfoType info_type,
                                    GITransfer transfer,
                                    GIDirection direction,
                                    gint c_arg_index,
                                    gint py_arg_index)
{
    PyGIInterfaceCache *iface_cache = NULL;
    PyGIArgCache *arg_cache = NULL;

    /* Callbacks are special cased */
    if (info_type != GI_INFO_TYPE_CALLBACK) {
        iface_cache = _interface_cache_new_from_interface_info(iface_info);

        arg_cache = (PyGIArgCache *)iface_cache;
        if (arg_cache == NULL)
            return NULL;
    }

    switch (info_type) {
        case GI_INFO_TYPE_UNION:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_union_setup(arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_union_setup(arg_cache, transfer);

            break;
        case GI_INFO_TYPE_STRUCT:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_struct_setup(arg_cache,
                                                    iface_info,
                                                    transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_struct_setup(arg_cache,
                                                     iface_info,
                                                     transfer);

            break;
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_object_setup(arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_object_setup(arg_cache, transfer);

            break;
        case GI_INFO_TYPE_BOXED:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_boxed_setup(arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_boxed_setup(arg_cache, transfer);

            break;
        case GI_INFO_TYPE_CALLBACK:
            {
                PyGICallbackCache *callback_cache;

                if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                    _arg_cache_out_interface_callback_setup();
                    return NULL;
                }

                callback_cache =
                    _callback_cache_new_from_arg_info(arg_info,
                                                      iface_info,
                                                      function_cache->is_method ? 1: 0);

                arg_cache = (PyGIArgCache *)callback_cache;
                if (arg_cache == NULL)
                    return NULL;

                if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                    _arg_cache_in_interface_callback_setup(arg_cache, function_cache);

                break;
            }
        case GI_INFO_TYPE_ENUM:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_enum_setup(arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_enum_setup(arg_cache, transfer);

            break;
        case GI_INFO_TYPE_FLAGS:
            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_interface_flags_setup(arg_cache, transfer);

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_interface_flags_setup(arg_cache, transfer);

            break;
        default:
            g_assert_not_reached();
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = GI_TYPE_TAG_INTERFACE;
        arg_cache->py_arg_index = py_arg_index;
        arg_cache->c_arg_index = c_arg_index;

        if (iface_cache != NULL) {
            g_base_info_ref( (GIBaseInfo *)iface_info);
            iface_cache->interface_info = iface_info;
        }
    }

    return arg_cache;
}

PyGIArgCache *
_arg_cache_new_from_type_info (GITypeInfo *type_info,
                               PyGIFunctionCache *function_cache,
                               GIArgInfo *arg_info,
                               GITypeTag type_tag,
                               GITransfer transfer,
                               GIDirection direction,
                               gboolean is_caller_allocates,
                               gint c_arg_index,
                               gint py_arg_index)
{
    PyGIArgCache *arg_cache = NULL;

    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_void_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_void_setup(arg_cache);
           break;
       case GI_TYPE_TAG_BOOLEAN:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_boolean_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_boolean_setup(arg_cache);

           break;
       case GI_TYPE_TAG_INT8:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int8_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int8_setup(arg_cache);

           break;
       case GI_TYPE_TAG_UINT8:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint8_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint8_setup(arg_cache);

           break;
       case GI_TYPE_TAG_INT16:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int16_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int16_setup(arg_cache);

           break;
       case GI_TYPE_TAG_UINT16:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint16_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint16_setup(arg_cache);

           break;
       case GI_TYPE_TAG_INT32:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int32_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int32_setup(arg_cache);

           break;
       case GI_TYPE_TAG_UINT32:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint32_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint32_setup(arg_cache);

           break;
       case GI_TYPE_TAG_INT64:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_int64_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_int64_setup(arg_cache);

           break;
       case GI_TYPE_TAG_UINT64:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_uint64_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_uint64_setup(arg_cache);

           break;
       case GI_TYPE_TAG_FLOAT:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_float_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_float_setup(arg_cache);

           break;
       case GI_TYPE_TAG_DOUBLE:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_double_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_double_setup(arg_cache);

           break;
       case GI_TYPE_TAG_UNICHAR:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_unichar_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_unichar_setup(arg_cache);

           break;
       case GI_TYPE_TAG_GTYPE:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_gtype_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_gtype_setup(arg_cache);

           break;
       case GI_TYPE_TAG_UTF8:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_utf8_setup(arg_cache, transfer);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_utf8_setup(arg_cache, transfer);

           break;
       case GI_TYPE_TAG_FILENAME:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_filename_setup(arg_cache, transfer);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_filename_setup(arg_cache, transfer);

           break;
       case GI_TYPE_TAG_ARRAY:
           {
               PyGISequenceCache *seq_cache =
                   _sequence_cache_new_from_type_info(type_info,
                                                      direction,
                                                      transfer,
                                                      (function_cache->is_method ? 1: 0));

               arg_cache = (PyGIArgCache *)seq_cache;
               if (arg_cache == NULL)
                   break;

               if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                   _arg_cache_in_array_setup(arg_cache,
                                             function_cache,
                                             type_info,
                                             transfer,
                                             direction);

               if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                   _arg_cache_out_array_setup(arg_cache,
                                              function_cache,
                                              type_info,
                                              transfer,
                                              c_arg_index);

               break;
           }
       case GI_TYPE_TAG_GLIST:
           {
               PyGISequenceCache *seq_cache =
                   _sequence_cache_new_from_type_info(type_info,
                                                      direction,
                                                      transfer,
                                                      (function_cache->is_method ? 1: 0));

               arg_cache = (PyGIArgCache *)seq_cache;
               if (arg_cache == NULL)
                   break;

               if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                   _arg_cache_in_glist_setup(arg_cache, transfer);

               if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                   _arg_cache_out_glist_setup(arg_cache, transfer);


               break;
           }
       case GI_TYPE_TAG_GSLIST:
           {
               PyGISequenceCache *seq_cache =
                   _sequence_cache_new_from_type_info(type_info,
                                                      direction,
                                                      transfer,
                                                      (function_cache->is_method ? 1: 0));

               arg_cache = (PyGIArgCache *)seq_cache;
               if (arg_cache == NULL)
                   break;

               if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
                   _arg_cache_in_gslist_setup(arg_cache, transfer);

               if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
                   _arg_cache_out_gslist_setup(arg_cache, transfer);

               break;
            }
       case GI_TYPE_TAG_GHASH:
           arg_cache =
               (PyGIArgCache *)_hash_cache_new_from_type_info(type_info,
                                                              direction,
                                                              transfer);

           if (arg_cache == NULL)
                   break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_ghash_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
               _arg_cache_out_ghash_setup(arg_cache);
           }

           break;
       case GI_TYPE_TAG_INTERFACE:
           {
               GIInterfaceInfo *interface_info = g_type_info_get_interface(type_info);
               GIInfoType info_type = g_base_info_get_type( (GIBaseInfo *) interface_info);
               arg_cache = _arg_cache_new_from_interface_info(interface_info,
                                                              function_cache,
                                                              arg_info,
                                                              info_type,
                                                              transfer,
                                                              direction,
                                                              c_arg_index,
                                                              py_arg_index);

               g_base_info_unref( (GIBaseInfo *) interface_info);
               break;
           }
       case GI_TYPE_TAG_ERROR:
           arg_cache = _arg_cache_new();
           if (arg_cache == NULL)
               break;

           if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
               _arg_cache_in_gerror_setup(arg_cache);

           if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
               _arg_cache_out_gerror_setup(arg_cache);

           break;
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = type_tag;
        arg_cache->py_arg_index = py_arg_index;
        arg_cache->c_arg_index = c_arg_index;
        arg_cache->is_pointer = g_type_info_is_pointer(type_info);
        g_base_info_ref( (GIBaseInfo *) type_info);
        arg_cache->type_info = type_info;
    }

    return arg_cache;
}

static inline gboolean
_args_cache_generate(GIFunctionInfo *function_info,
                     PyGIFunctionCache *function_cache)
{
    int arg_index = 0;
    int i;
    GITypeInfo *return_info;
    GITypeTag return_type_tag;
    GITransfer return_transfer;
    PyGIArgCache *return_cache;
    /* cache the return arg */
    return_info =
        g_callable_info_get_return_type( (GICallableInfo *)function_info);
    return_transfer =
        g_callable_info_get_caller_owns( (GICallableInfo *)function_info);
    return_type_tag = g_type_info_get_tag(return_info);
    return_cache =
        _arg_cache_new_from_type_info(return_info,
                                      function_cache,
                                      NULL,
                                      return_type_tag,
                                      return_transfer,
                                      GI_DIRECTION_OUT,
                                      FALSE,
                                      -1,
                                      -1);

    function_cache->return_cache = return_cache;
    g_base_info_unref(return_info);

    /* first arg is the instance */
    if (function_cache->is_method) {
        GIInterfaceInfo *interface_info;
        PyGIArgCache *instance_cache;
        GIInfoType info_type;

        interface_info = g_base_info_get_container ( (GIBaseInfo *) function_info);
        info_type = g_base_info_get_type(interface_info);

        instance_cache =
            _arg_cache_new_from_interface_info(interface_info,
                                               function_cache,
                                               NULL,
                                               info_type,
                                               GI_TRANSFER_NOTHING,
                                               GI_DIRECTION_IN,
                                               arg_index,
                                               0);

        g_base_info_unref( (GIBaseInfo *) interface_info);

        if (instance_cache == NULL)
            return FALSE;

        function_cache->args_cache[arg_index] = instance_cache;

        arg_index++;
        function_cache->n_in_args++;
        function_cache->n_py_args++;
    }


    for (i=0; arg_index < function_cache->n_args; arg_index++, i++) {
        PyGIArgCache *arg_cache = NULL;
        GIArgInfo *arg_info;
        GITypeInfo *type_info;
        GIDirection direction;
        GITransfer transfer;
        GITypeTag type_tag;
        gboolean is_caller_allocates;
        gint py_arg_index = -1;

        arg_info =
            g_callable_info_get_arg( (GICallableInfo *) function_info, i);

        direction = g_arg_info_get_direction(arg_info);
        transfer = g_arg_info_get_ownership_transfer(arg_info);
        type_info = g_arg_info_get_type(arg_info);
        type_tag = g_type_info_get_tag(type_info);

        /* must be an aux arg filled in by its owner
         * fill in it's c_arg_index, add to the in count
         * and continue
         */
        if (function_cache->args_cache[arg_index] != NULL) {
            arg_cache = function_cache->args_cache[arg_index];
            if (arg_cache->aux_type == PYGI_AUX_TYPE_HAS_PYARG) {
                arg_cache->py_arg_index = function_cache->n_py_args;
                function_cache->n_py_args++;
            }

            if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
                arg_cache->c_arg_index = function_cache->n_in_args;
                function_cache->n_in_args++;
            }

            if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
                function_cache->n_out_args++;
                function_cache->n_out_aux_args++;
            }

            g_base_info_unref( (GIBaseInfo *)arg_info);
            continue;
        }

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            py_arg_index = function_cache->n_py_args;
            function_cache->n_in_args++;
            function_cache->n_py_args++;
        }

        arg_cache =
            _arg_cache_new_from_type_info(type_info,
                                          function_cache,
                                          arg_info,
                                          type_tag,
                                          transfer,
                                          direction,
                                          FALSE,
                                          arg_index,
                                          py_arg_index);

        if (arg_cache == NULL)
            goto arg_err;

        arg_cache->allow_none = g_arg_info_may_be_null (arg_info);

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            function_cache->n_out_args++;
            is_caller_allocates = g_arg_info_is_caller_allocates (arg_info);
            if (is_caller_allocates) {
                PyErr_Format(PyExc_NotImplementedError,
                             "Caller allocates is not fully implemented yet");

                goto arg_err;
            }

            if (arg_cache == NULL)
                goto arg_err;

            function_cache->out_args =
                g_slist_append(function_cache->out_args, arg_cache);
        }

        function_cache->args_cache[arg_index] = arg_cache;
        g_base_info_unref( (GIBaseInfo *)type_info);
        g_base_info_unref( (GIBaseInfo *)arg_info);

        continue;
arg_err:
        g_base_info_unref( (GIBaseInfo *)type_info);
        g_base_info_unref( (GIBaseInfo *)arg_info);
        return FALSE;
    }
    return TRUE;
}

PyGIFunctionCache *
_pygi_function_cache_new (GIFunctionInfo *function_info)
{
    PyGIFunctionCache *fc = _function_cache_new_from_function_info(function_info);
    if (fc == NULL)
        return NULL;

    if (!_args_cache_generate(function_info, fc))
        goto err;

    return fc;
err:
    _pygi_function_cache_free(fc);
    return NULL;
}
