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
#include "pygi-cache.h"
#include "pygi-argument.h"
#include <girepository.h>

PyGIArgCache * _arg_cache_in_new_from_type_info (GITypeInfo *type_info,
                                  PyGIFunctionCache *function_cache,
                                  GITypeTag type_tag,
                                  GITransfer transfer,
                                  GIDirection direction,
                                  gint c_arg_index,
                                  gint py_arg_index);
/* cleanup */
static inline void
_interface_cache_free_func (PyGIInterfaceCache *cache)
{
    if (cache != NULL) {
        Py_XDECREF(cache->py_type);
        g_slice_free(PyGIInterfaceCache, cache);
    }
}

static inline void
_pygi_hash_cache_free (PyGIHashCache *cache)
{
    if (cache != NULL)
        g_slice_free(PyGIHashCache, cache);
}

static void
_sequence_cache_free_func (PyGISequenceCache *cache)
{
    if (cache != NULL)
        g_slice_free(PyGISequenceCache, cache);
}

static void
_callback_cache_free_func (PyGICallbackCache *cache)
{
    if (cache != NULL)
        g_slice_free(PyGICallbackCache, cache);
}

void
_pygi_arg_cache_free (PyGIArgCache *cache)
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

void
_pygi_function_cache_free (PyGIFunctionCache *cache)
{
    int i;

    if (cache == NULL)
        return;

    g_slist_free(cache->in_args);
    g_slist_free(cache->out_args);
    for (i = 0; i < cache->n_args; i++) {
        PyGIArgCache *tmp = cache->args_cache[i];
        _pygi_arg_cache_free(tmp);
    }
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
    flags = g_function_info_get_flags(function_info);
    fc->is_method = flags & GI_FUNCTION_IS_METHOD;
    fc->is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;
    fc->n_args = g_callable_info_get_n_args ( (GICallableInfo *) function_info) + (fc->is_method ? 1: 0);
    if (fc->n_args > 0)
        fc->args_cache = g_slice_alloc0(fc->n_args * sizeof(PyGIArgCache *));

    return fc;
}

static inline PyGISequenceCache *
_sequence_cache_new_from_type_info(GITypeInfo *type_info)
{
    PyGISequenceCache *sc;
    GITypeInfo *item_type_info;
    GITypeTag item_type_tag;

    sc = g_slice_new0(PyGISequenceCache);

    sc->fixed_size = -1;
    sc->len_arg_index = -1;
    sc->is_zero_terminated = g_type_info_is_zero_terminated(type_info);
    if (!sc->is_zero_terminated)
        sc->fixed_size = g_type_info_get_array_fixed_size(type_info);
    if (sc->fixed_size < 0)
        sc->len_arg_index = g_type_info_get_array_length (type_info);

    item_type_info = g_type_info_get_param_type (type_info, 0);
    item_type_tag = g_type_info_get_tag (item_type_info);

    /* FIXME: support out also */
    sc->item_cache = _arg_cache_in_new_from_type_info(item_type_info,
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
    g_base_info_unref( (GIBaseInfo *) item_type_info);

    ((PyGIArgCache *)sc)->destroy_notify = (GDestroyNotify)_sequence_cache_free_func;

    return sc;
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
_arg_cache_new_for_in_array(GITypeInfo *type_info,
                            GITransfer transfer)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info);
    arg_cache->in_marshaller = _pygi_marshal_in_array;

    /* arg_cache->cleanup = _pygi_cleanup_array; */
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_glist(GITypeInfo *type_info,
                                      GITransfer transfer)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info);
    arg_cache->in_marshaller = _pygi_marshal_in_glist;
    /* arg_cache->cleanup = */

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_gslist(GITypeInfo *type_info,
                                       GITransfer transfer)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)_sequence_cache_new_from_type_info(type_info);
    arg_cache->in_marshaller = _pygi_marshal_in_gslist;
    /* arg_cache->cleanup = */

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_ghash(GITypeInfo *type_info)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_ghash;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_gerror(void)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_gerror;
    arg_cache->is_aux = TRUE;
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_union(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_inteface_union;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_struct(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_interface_struct;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_object(GITransfer transfer)
{
    PyGIArgCache *arg_cache = _arg_cache_new();
    arg_cache->in_marshaller = _pygi_marshal_in_interface_object;
    if (transfer == GI_TRANSFER_EVERYTHING)
        arg_cache->cleanup = (GDestroyNotify)g_object_unref;

    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_boxed(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_boxed;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_callback(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_callback;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_enum(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_enum;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

static inline PyGIArgCache *
_arg_cache_new_for_in_interface_flags(void)
{
    PyGIArgCache *arg_cache = NULL;
    /*arg_cache->in_marshaller = _pygi_marshal_in_flags;*/
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return arg_cache;
}

PyGIArgCache *
_arg_cache_in_new_from_interface_info (GIInterfaceInfo *iface_info,
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
            arg_cache = _arg_cache_new_for_in_interface_struct();
            break;
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            arg_cache = _arg_cache_new_for_in_interface_object(transfer);
            break;
        case GI_INFO_TYPE_BOXED:
            arg_cache = _arg_cache_new_for_in_interface_boxed();
            break;
        case GI_INFO_TYPE_CALLBACK:
            arg_cache = _arg_cache_new_for_in_interface_callback();
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
}


PyGIArgCache *
_arg_cache_in_new_from_type_info (GITypeInfo *type_info,
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
           arg_cache = _arg_cache_new_for_in_array(type_info,
                                                   transfer);
           break;
       case GI_TYPE_TAG_INTERFACE:
           {
               GIInterfaceInfo *interface_info = g_type_info_get_interface(type_info);
               GIInfoType info_type = g_base_info_get_type( (GIBaseInfo *) interface_info);
               arg_cache = _arg_cache_in_new_from_interface_info(interface_info,
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
    /* first arg is the instance */
    if (function_cache->is_method) {
        GIInterfaceInfo *interface_info;
        PyGIArgCache *instance_cache;
        GIInfoType info_type;

        interface_info = g_base_info_get_container ( (GIBaseInfo *) function_info);
        info_type = g_base_info_get_type(interface_info);

        instance_cache =
            _arg_cache_in_new_from_interface_info(interface_info,
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
    }

    for (arg_index; arg_index < function_cache->n_args; arg_index++) {
        PyGIArgCache *arg_cache = NULL;
        GIArgInfo *arg_info;
        GITypeInfo *type_info;
        GIDirection direction;
        GITransfer transfer;
        GITypeTag type_tag;
        gint py_arg_index;

        /* must be an aux arg filled in by its owner so skip */
        if (function_cache->args_cache[arg_index] != NULL)
            continue;

        arg_info =
            g_callable_info_get_arg( (GICallableInfo *) function_info, arg_index);

        direction = g_arg_info_get_direction(arg_info);
        transfer = g_arg_info_get_ownership_transfer(arg_info);
        type_info = g_arg_info_get_type(arg_info);
        type_tag = g_type_info_get_tag(type_info);

        switch(direction) {
            case GI_DIRECTION_IN:
                py_arg_index = function_cache->n_in_args;
                function_cache->n_in_args++;

                arg_cache =
                    _arg_cache_in_new_from_type_info(type_info,
                                                     function_cache,
                                                     type_tag,
                                                     transfer,
                                                     direction,
                                                     arg_index,
                                                     py_arg_index);

                if (arg_cache == NULL)
                    goto arg_err;

                function_cache->in_args =
                    g_slist_append(function_cache->in_args, arg_cache);
                break;

            case GI_DIRECTION_OUT:
                function_cache->n_out_args++;
                PyErr_Format(PyExc_NotImplementedError,
                             "Out caching is not fully implemented yet");

                goto arg_err;

            case GI_DIRECTION_INOUT:
                PyErr_Format(PyExc_NotImplementedError,
                             "In/Out caching is not fully implemented yet");
                goto arg_err;

        }

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
