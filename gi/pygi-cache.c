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

PyGIArgCache * _arg_cache_in_new_from_type_info (GITypeInfo *type_info,
                                                 GIArgInfo *arg_info,
                                                 PyGIFunctionCache *function_cache,
                                                 GITypeTag type_tag,
                                                 GITransfer transfer,
                                                 GIDirection direction,
                                                 gint c_arg_index,
                                                 gint py_arg_index);
/* cleanup */
void
_pygi_arg_cache_free(PyGIArgCache *cache)
{
    if (cache == NULL)
        return;

    if (cache->arg_info != NULL)
        g_base_info_unref(cache->arg_info);
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
        g_slice_free(PyGIInterfaceCache, cache);
        if (cache->type_name != NULL)
            g_free(cache->type_name);
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
    if (cache != NULL)
        g_slice_free(PyGICallbackCache, cache);
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

    ic->type_name == _pygi_g_base_info_get_fullname(iface_info);
    return ic;
}

static inline PyGISequenceCache *
_sequence_cache_new_from_type_info(GITypeInfo *type_info,
                                   gint aux_offset)
{
    PyGISequenceCache *sc;
    GITypeInfo *item_type_info;
    GITypeTag item_type_tag;

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

    /* FIXME: support out also */
    sc->item_cache = _arg_cache_in_new_from_type_info(item_type_info,
                                                      NULL,
                                                      NULL,
                                                      item_type_tag,
                                                      GI_TRANSFER_EVERYTHING,
                                                      GI_DIRECTION_IN,
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
_hash_cache_new_from_type_info(GITypeInfo *type_info)
{
    PyGIHashCache *hc;
    GITypeInfo *key_type_info;
    GITypeTag key_type_tag;
    GITypeInfo *value_type_info;
    GITypeTag value_type_tag;

    hc = g_slice_new0(PyGIHashCache);
    ((PyGIArgCache *)hc)->destroy_notify = (GDestroyNotify)_hash_cache_free_func;
    key_type_info = g_type_info_get_param_type (type_info, 0);
    key_type_tag = g_type_info_get_tag (key_type_info);
    value_type_info = g_type_info_get_param_type (type_info, 1);
    value_type_tag = g_type_info_get_tag (value_type_info);

    hc->key_cache = _arg_cache_in_new_from_type_info(key_type_info,
                                                     NULL,
                                                     NULL,
                                                     key_type_tag,
                                                     GI_TRANSFER_EVERYTHING,
                                                     GI_DIRECTION_IN,
                                                     0, 0);

    if (hc->key_cache == NULL) {
        _pygi_arg_cache_free((PyGIArgCache *)hc);
        return NULL;
    }

    hc->value_cache = _arg_cache_in_new_from_type_info(value_type_info,
                                                       NULL,
                                                       NULL,
                                                       value_type_tag,
                                                       GI_TRANSFER_EVERYTHING,
                                                       GI_DIRECTION_IN,
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
                                  gint aux_offset)
{
   PyGICallbackCache *cc;

   cc = g_slice_new0(PyGICallbackCache);
   cc->user_data_index = g_arg_info_get_closure(arg_info) + aux_offset;
   cc->destroy_notify_index = g_arg_info_get_destroy(arg_info) + aux_offset;
   cc->scope = g_arg_info_get_scope(arg_info);

   return cc;
}

static inline PyGIArgCache *
_arg_cache_new(void)
{
    return g_slice_new0(PyGIArgCache);
}
/* process in args */

static inline PyGIArgCache *
_arg_cache_new_for_in_void(void)
{
     PyGIArgCache *arg_cache = _arg_cache_new();
     arg_cache->in_marshaller = _pygi_marshal_in_void;

     return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_boolean(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_boolean;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_int8(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_int8;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_uint8(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_uint8;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_int16(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_int16;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_uint16(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_uint16;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_int32(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_int32;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_uint32(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_uint32;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_int64(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_int64;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_uint64(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_uint64;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_float(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_float;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_double(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_double;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_unichar(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_unichar;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_gtype(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_gtype;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_utf8(GITransfer transfer)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_utf8;
    if (arg_cache->transfer == GI_TRANSFER_NOTHING)
        arg_cache->cleanup = g_free;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_filename(GITransfer transfer)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_filename;
    if (arg_cache->transfer == GI_TRANSFER_NOTHING)
        arg_cache->cleanup = g_free;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_array(PyGIFunctionCache *function_cache,
                            GITypeInfo *type_info,
                            GITransfer transfer)
{
    PyGISequenceCache *seq_cache = _sequence_cache_new_from_type_info(type_info,                                                                      (function_cache->is_method ? 1: 0));
    PyGIArgCache *arg_cache = (PyGIArgCache *)seq_cache;

    seq_cache->array_type = g_type_info_get_array_type(type_info);

    if (seq_cache->len_arg_index >= 0) {
        PyGIArgCache *aux_cache = _arg_cache_new();
        aux_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
        if (function_cache->args_cache[seq_cache->len_arg_index] != NULL) {
            PyGIArgCache *invalid_cache = function_cache->args_cache[seq_cache->len_arg_index];
            arg_cache->c_arg_index = invalid_cache->c_arg_index;
            _pygi_arg_cache_free(invalid_cache);
        }

        function_cache->args_cache[seq_cache->len_arg_index] = aux_cache;
    }

    arg_cache->in_marshaller = _pygi_marshal_in_array;

    /* arg_cache->cleanup = _pygi_cleanup_array; */
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_glist(GITypeInfo *type_info,
                            GITransfer transfer)
{
    PyGIArgCache *arg_cache =
        (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info, 0);
    arg_cache->in_marshaller = _pygi_marshal_in_glist;
    /* arg_cache->cleanup = */

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_gslist(GITypeInfo *type_info,
                                       GITransfer transfer)
{
    PyGIArgCache *arg_cache =
        (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info, 0);
    arg_cache->in_marshaller = _pygi_marshal_in_gslist;
    /* arg_cache->cleanup = */

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_ghash(GITypeInfo *type_info)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_hash_cache_new_from_type_info(type_info);
    arg_cache->in_marshaller = _pygi_marshal_in_ghash;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_gerror(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_gerror;
    arg_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_union(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_inteface_union;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for In Interface Union is not fully implemented yet");
    return arg_cache;
}


static void
_g_slice_free_gvalue_func(GValue *value) {
    g_slice_free(GValue, value);
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_struct(GIInterfaceInfo *iface_info,
                                       GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = _interface_cache_new_from_interface_info(iface_info);
    PyGIArgCache *arg_cache = (PyGIArgCache *)iface_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign( (GIStructInfo*)iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;
    if (iface_cache->g_type == G_TYPE_VALUE)
        arg_cache->cleanup = _g_slice_free_gvalue_func;
    if (iface_cache->g_type == G_TYPE_CLOSURE)
        arg_cache->cleanup = g_closure_unref;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_object(GIInterfaceInfo *iface_info,
                                       GITransfer transfer)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_interface_cache_new_from_interface_info(iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_object;
    if (transfer == GI_TRANSFER_EVERYTHING)
        arg_cache->cleanup = (GDestroyNotify)g_object_unref;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_boxed(GIInterfaceInfo *iface_info,
                                      GITransfer transfer)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_interface_cache_new_from_interface_info(iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_boxed;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_callback(PyGIFunctionCache *function_cache,
                                         GIArgInfo *arg_info)
{
    PyGICallbackCache *callback_cache = _callback_cache_new_from_arg_info(arg_info, function_cache->is_method ? 1: 0);
    PyGIArgCache *arg_cache = (PyGIArgCache *)callback_cache;
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
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_enum(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_enum;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for In Interface ENum is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_flags(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_flags;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for In Interface Flags is not fully implemented yet");
    return arg_cache;
}

PyGIArgCache *
_arg_cache_in_new_from_interface_info (GIInterfaceInfo *iface_info,
                                       GIArgInfo *arg_info,
                                       PyGIFunctionCache *function_cache,
                                       GIInfoType info_type,
                                       GITransfer transfer,
                                       GIDirection direction,
                                       gint c_arg_index,
                                       gint py_arg_index)
{
    PyGIArgCache *arg_cache = NULL;

    switch (info_type) {
        case GI_INFO_TYPE_UNION:
            arg_cache = _arg_cache_new_for_in_interface_union();
            break;
        case GI_INFO_TYPE_STRUCT:
            arg_cache = _arg_cache_new_for_in_interface_struct(iface_info,
                                                               transfer);
            break;
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            arg_cache = _arg_cache_new_for_in_interface_object(iface_info,
                                                               transfer);
            break;
        case GI_INFO_TYPE_BOXED:
            arg_cache = _arg_cache_new_for_in_interface_boxed(iface_info,
                                                              transfer);
            break;
        case GI_INFO_TYPE_CALLBACK:
            arg_cache = _arg_cache_new_for_in_interface_callback(function_cache,
                                                                 arg_info);
            break;
        case GI_INFO_TYPE_ENUM:
            arg_cache = _arg_cache_new_for_in_interface_enum();
            break;
        case GI_INFO_TYPE_FLAGS:
            arg_cache = _arg_cache_new_for_in_interface_flags();
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
    }

    return arg_cache;
}

/* process out args */

static inline PyGIArgCache *
_arg_cache_new_for_out_void(void)
{
     PyGIArgCache *arg_cache = _arg_cache_new();
     arg_cache->out_marshaller = _pygi_marshal_out_void;

     return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_boolean(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_boolean;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_int8(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_int8;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_uint8(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_uint8;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_int16(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_int16;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_uint16(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_uint16;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_int32(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_int32;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_uint32(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_uint32;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_int64(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_int64;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_uint64(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_uint64;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_float(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_float;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_double(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_double;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_unichar(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_unichar;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_gtype(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_gtype;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_utf8(GITransfer transfer)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_utf8;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_filename(GITransfer transfer)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_filename;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_array(GITypeInfo *type_info,
                            GITransfer transfer)
{
    PyGIArgCache *arg_cache =
        (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info, 0);
    arg_cache->out_marshaller = _pygi_marshal_out_array;

    /* arg_cache->cleanup = _pygi_cleanup_array; */
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_glist(GITypeInfo *type_info,
                                      GITransfer transfer)
{
    PyGIArgCache *arg_cache =
       (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info, 0);
    arg_cache->out_marshaller = _pygi_marshal_out_glist;
    /* arg_cache->cleanup = */

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_gslist(GITypeInfo *type_info,
                                       GITransfer transfer)
{
    PyGIArgCache *arg_cache =
        (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info, 0);
    arg_cache->out_marshaller = _pygi_marshal_out_gslist;
    /* arg_cache->cleanup = */

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_ghash(GITypeInfo *type_info)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->out_marshaller = _pygi_marshal_out_ghash;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for Out GHash is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_gerror(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->out_marshaller = _pygi_marshal_out_gerror;
    arg_cache->aux_type = PYGI_AUX_TYPE_IGNORE;
    return arg_cache;
}
static inline PyGIArgCache *
_arg_cache_new_for_out_interface_struct(GIInterfaceInfo *iface_info,
                                        GITransfer transfer)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for Out Interface Struct is not fully implemented yet");
    return FALSE;
    PyGIInterfaceCache *iface_cache = _interface_cache_new_from_interface_info(iface_info);
    PyGIArgCache *arg_cache = (PyGIArgCache *)iface_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign( (GIStructInfo*)iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;
    if (iface_cache->g_type == G_TYPE_VALUE)
        arg_cache->cleanup = _g_slice_free_gvalue_func;
    if (iface_cache->g_type == G_TYPE_CLOSURE)
        arg_cache->cleanup = g_closure_unref;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_interface_object(GIInterfaceInfo *iface_info,
                                        GITransfer transfer)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_interface_cache_new_from_interface_info(iface_info);
    arg_cache->out_marshaller = _pygi_marshal_out_interface_object;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_interface_boxed(GIInterfaceInfo *iface_info,
                                      GITransfer transfer)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for Out Interface Boxed is not fully implemented yet");
    return FALSE;

    PyGIArgCache *arg_cache = (PyGIArgCache *)_interface_cache_new_from_interface_info(iface_info);
    arg_cache->in_marshaller = _pygi_marshal_in_interface_boxed;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_interface_callback(void)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Callback returns are not supported");
    return FALSE;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_interface_enum(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_enum;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for Out Interface ENum is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_out_interface_union(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_enum;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for Out Interface Unions is not fully implemented yet");
    return arg_cache;
}
static inline PyGIArgCache *
_arg_cache_new_for_out_interface_flags(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_flags;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for Out Interface Flags is not fully implemented yet");
    return arg_cache;
}

PyGIArgCache *
_arg_cache_out_new_from_interface_info (GIInterfaceInfo *iface_info,
                                        PyGIFunctionCache *function_cache,
                                        GIInfoType info_type,
                                        GITransfer transfer,
                                        GIDirection direction,
                                        gint c_arg_index)
{
    PyGIArgCache *arg_cache = NULL;

    switch (info_type) {
        case GI_INFO_TYPE_UNION:
            arg_cache = _arg_cache_new_for_out_interface_union();
            break;
        case GI_INFO_TYPE_STRUCT:
            arg_cache = _arg_cache_new_for_out_interface_struct(iface_info,
                                                               transfer);
            break;
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            arg_cache = _arg_cache_new_for_out_interface_object(iface_info,
                                                                transfer);
            break;
        case GI_INFO_TYPE_BOXED:
            arg_cache = _arg_cache_new_for_out_interface_boxed(iface_info,
                                                               transfer);
            break;
        case GI_INFO_TYPE_CALLBACK:
            arg_cache = _arg_cache_new_for_out_interface_callback();
            break;
        case GI_INFO_TYPE_ENUM:
            arg_cache = _arg_cache_new_for_out_interface_enum();
            break;
        case GI_INFO_TYPE_FLAGS:
            arg_cache = _arg_cache_new_for_out_interface_flags();
            break;
        default:
            g_assert_not_reached();
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = GI_TYPE_TAG_INTERFACE;
        arg_cache->c_arg_index = c_arg_index;
    }

    return arg_cache;
}

PyGIArgCache *
_arg_cache_out_new_from_type_info (GITypeInfo *type_info,
                                   PyGIFunctionCache *function_cache,
                                   GITypeTag type_tag,
                                   GITransfer transfer,
                                   GIDirection direction,
                                   gint c_arg_index)
{
    PyGIArgCache *arg_cache = NULL;

    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
           arg_cache = _arg_cache_new_for_out_void();
           break;
       case GI_TYPE_TAG_BOOLEAN:
           arg_cache = _arg_cache_new_for_out_boolean();
           break;
       case GI_TYPE_TAG_INT8:
           arg_cache = _arg_cache_new_for_out_int8();
           break;
       case GI_TYPE_TAG_UINT8:
           arg_cache = _arg_cache_new_for_out_uint8();
           break;
       case GI_TYPE_TAG_INT16:
           arg_cache = _arg_cache_new_for_out_uint16();
           break;
       case GI_TYPE_TAG_UINT16:
           arg_cache = _arg_cache_new_for_out_uint16();
           break;
       case GI_TYPE_TAG_INT32:
           arg_cache = _arg_cache_new_for_out_int32();
           break;
       case GI_TYPE_TAG_UINT32:
           arg_cache = _arg_cache_new_for_out_uint32();
           break;
       case GI_TYPE_TAG_INT64:
           arg_cache = _arg_cache_new_for_out_int64();
           break;
       case GI_TYPE_TAG_UINT64:
           arg_cache = _arg_cache_new_for_out_uint64();
           break;
       case GI_TYPE_TAG_FLOAT:
           arg_cache = _arg_cache_new_for_out_float();
           break;
       case GI_TYPE_TAG_DOUBLE:
           arg_cache = _arg_cache_new_for_out_double();
           break;
       case GI_TYPE_TAG_UNICHAR:
           arg_cache = _arg_cache_new_for_out_unichar();
           break;
       case GI_TYPE_TAG_GTYPE:
           arg_cache = _arg_cache_new_for_out_gtype();
           break;
       case GI_TYPE_TAG_UTF8:
           arg_cache = _arg_cache_new_for_out_utf8(transfer);
           break;
       case GI_TYPE_TAG_FILENAME:
           arg_cache = _arg_cache_new_for_out_filename(transfer);
           break;
       case GI_TYPE_TAG_ARRAY:
           arg_cache = _arg_cache_new_for_out_array(type_info,
                                                   transfer);
           break;
       case GI_TYPE_TAG_INTERFACE:
           {
               GIInterfaceInfo *interface_info = g_type_info_get_interface(type_info);
               GIInfoType info_type = g_base_info_get_type( (GIBaseInfo *) interface_info);
               arg_cache = _arg_cache_out_new_from_interface_info(interface_info,
                                                                 function_cache,
                                                                 info_type,
                                                                 transfer,
                                                                 direction,
                                                                 c_arg_index);

               g_base_info_unref( (GIBaseInfo *) interface_info);
               return arg_cache;
           }
       case GI_TYPE_TAG_GLIST:
           arg_cache = _arg_cache_new_for_out_glist(type_info,
                                                   transfer);
           break;
       case GI_TYPE_TAG_GSLIST:
           arg_cache = _arg_cache_new_for_out_gslist(type_info,
                                                    transfer);
           break;
       case GI_TYPE_TAG_GHASH:
           arg_cache = _arg_cache_new_for_out_ghash(type_info);
           break;
       case GI_TYPE_TAG_ERROR:
           arg_cache = _arg_cache_new_for_out_gerror();
           break;
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = type_tag;
        arg_cache->c_arg_index = c_arg_index;
    }

    return arg_cache;
}

PyGIArgCache *
_arg_cache_in_new_from_type_info (GITypeInfo *type_info,
                                  GIArgInfo *arg_info,
                                  PyGIFunctionCache *function_cache,
                                  GITypeTag type_tag,
                                  GITransfer transfer,
                                  GIDirection direction,
                                  gint c_arg_index,
                                  gint py_arg_index)
{
    PyGIArgCache *arg_cache = NULL;

    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
           arg_cache = _arg_cache_new_for_in_void();
           break;
       case GI_TYPE_TAG_BOOLEAN:
           arg_cache = _arg_cache_new_for_in_boolean();
           break;
       case GI_TYPE_TAG_INT8:
           arg_cache = _arg_cache_new_for_in_int8();
           break;
       case GI_TYPE_TAG_UINT8:
           arg_cache = _arg_cache_new_for_in_uint8();
           break;
       case GI_TYPE_TAG_INT16:
           arg_cache = _arg_cache_new_for_in_uint16();
           break;
       case GI_TYPE_TAG_UINT16:
           arg_cache = _arg_cache_new_for_in_uint16();
           break;
       case GI_TYPE_TAG_INT32:
           arg_cache = _arg_cache_new_for_in_int32();
           break;
       case GI_TYPE_TAG_UINT32:
           arg_cache = _arg_cache_new_for_in_uint32();
           break;
       case GI_TYPE_TAG_INT64:
           arg_cache = _arg_cache_new_for_in_int64();
           break;
       case GI_TYPE_TAG_UINT64:
           arg_cache = _arg_cache_new_for_in_uint64();
           break;
       case GI_TYPE_TAG_FLOAT:
           arg_cache = _arg_cache_new_for_in_float();
           break;
       case GI_TYPE_TAG_DOUBLE:
           arg_cache = _arg_cache_new_for_in_double();
           break;
       case GI_TYPE_TAG_UNICHAR:
           arg_cache = _arg_cache_new_for_in_unichar();
           break;
       case GI_TYPE_TAG_GTYPE:
           arg_cache = _arg_cache_new_for_in_gtype();
           break;
       case GI_TYPE_TAG_UTF8:
           arg_cache = _arg_cache_new_for_in_utf8(transfer);
           break;
       case GI_TYPE_TAG_FILENAME:
           arg_cache = _arg_cache_new_for_in_filename(transfer);
           break;
       case GI_TYPE_TAG_ARRAY:
           arg_cache = _arg_cache_new_for_in_array(function_cache,
                                                   type_info,
                                                   transfer);
           break;
       case GI_TYPE_TAG_INTERFACE:
           {
               GIInterfaceInfo *interface_info = g_type_info_get_interface(type_info);
               GIInfoType info_type = g_base_info_get_type( (GIBaseInfo *) interface_info);
               arg_cache = _arg_cache_in_new_from_interface_info(interface_info,
                                                                 arg_info,
                                                                 function_cache,
                                                                 info_type,
                                                                 transfer,
                                                                 direction,
                                                                 c_arg_index,
                                                                 py_arg_index);

               g_base_info_unref( (GIBaseInfo *) interface_info);
               return arg_cache;
           }
       case GI_TYPE_TAG_GLIST:
           arg_cache = _arg_cache_new_for_in_glist(type_info,
                                                   transfer);
           break;
       case GI_TYPE_TAG_GSLIST:
           arg_cache = _arg_cache_new_for_in_gslist(type_info,
                                                    transfer);
           break;
       case GI_TYPE_TAG_GHASH:
           arg_cache = _arg_cache_new_for_in_ghash(type_info);
           break;
       case GI_TYPE_TAG_ERROR:
           arg_cache = _arg_cache_new_for_in_gerror();
           break;
    }

    if (arg_cache != NULL) {
        arg_cache->direction = direction;
        arg_cache->transfer = transfer;
        arg_cache->type_tag = type_tag;
        arg_cache->py_arg_index = py_arg_index;
        arg_cache->c_arg_index = c_arg_index;
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
    PyGIArgCache *return_cache;
    /* cache the return arg */
    return_info = g_callable_info_get_return_type( (GICallableInfo *)function_info);
    return_type_tag = g_type_info_get_tag(return_info);
    return_cache =
        _arg_cache_out_new_from_type_info(return_info,
                                          function_cache,
                                          return_type_tag,
                                          GI_TRANSFER_EVERYTHING,
                                          GI_DIRECTION_OUT,
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
            _arg_cache_in_new_from_interface_info(interface_info,
                                                  NULL,
                                                  function_cache,
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
        gint py_arg_index;

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
            arg_cache->c_arg_index = function_cache->n_in_args;
            function_cache->n_in_args++;
            continue;
        }

        arg_info =
            g_callable_info_get_arg( (GICallableInfo *) function_info, i);

        direction = g_arg_info_get_direction(arg_info);
        transfer = g_arg_info_get_ownership_transfer(arg_info);
        type_info = g_arg_info_get_type(arg_info);
        type_tag = g_type_info_get_tag(type_info);

        switch(direction) {
            case GI_DIRECTION_IN:
                py_arg_index = function_cache->n_py_args;
                function_cache->n_in_args++;
                function_cache->n_py_args++;

                arg_cache =
                    _arg_cache_in_new_from_type_info(type_info,
                                                     arg_info,
                                                     function_cache,
                                                     type_tag,
                                                     transfer,
                                                     direction,
                                                     arg_index,
                                                     py_arg_index);

                if (arg_cache == NULL)
                    goto arg_err;

                arg_cache->allow_none = g_arg_info_may_be_null (arg_info);
                break;

            case GI_DIRECTION_OUT:
                function_cache->n_out_args++;
                arg_cache =
                    _arg_cache_out_new_from_type_info(type_info,
                                                      function_cache,
                                                      type_tag,
                                                      transfer,
                                                      direction,
                                                      arg_index);

                if (arg_cache == NULL)
                    goto arg_err;

                function_cache->out_args =
                    g_slist_append(function_cache->out_args, arg_cache);

                break;

            case GI_DIRECTION_INOUT:
                PyErr_Format(PyExc_NotImplementedError,
                             "In/Out caching is not fully implemented yet");
                goto arg_err;

        }

        arg_cache->arg_info = arg_info;
        function_cache->args_cache[arg_index] = arg_cache;
        g_base_info_unref( (GIBaseInfo *) type_info);
        continue;
arg_err:
        g_base_info_unref( (GIBaseInfo *) type_info);
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
