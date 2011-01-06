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


/* cleanup */
static inline void
_pygi_interface_cache_free (PyGIInterfaceCache *cache)
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

static inline void
_pygi_sequence_cache_free (PyGISequenceCache *cache)
{
    if (cache != NULL)
        g_slice_free(PyGISequenceCache, cache);
}

static inline void
_pygi_callback_cache_free (PyGICallbackCache *cache)
{
    if (cache != NULL)
        g_slice_free(PyGICallbackCache, cache);
}

void
_pygi_arg_cache_clear (PyGIArgCache *cache);
{
    cache->is_aux = FALSE;
    cache->is_pointer = FALSE;
    cache->direction = 0;
    g_base_info_unref(cache->arg_info);
    
    cache->in_validator = NULL; 
    cache->in_marshaler = NULL; 
    cache->out_marshaler = NULL; 
    cache->cleanup = NULL; 

    _pygi_sequence_cache_free(cache->sequence_cache);
    cache->sequence_cache = NULL;
    _pygi_interface_cache_free(cache->interface_cache);
    cache->interface_cache = NULL;
    _pygi_hash_cache_free(cache->hash_cache);
    cache->hash_cache = NULL; 
    _pygi_callback_cache_free(cache->callback_cache);
    cache->callback_cache = NULL;

    gint c_arg_index = -1;
    gint py_arg_index = -1;
}

void
_pygi_function_cache_free (PyGIFunctionCache *cache)
{
    int i;

    g_slist_free(cache->in_args);
    g_slist_free(cache->out_args);
    for (i = 0; i < cache->n_args; i++) {
        PyGIArgCache *tmp = cache->args_cache[i];
        _pygi_arg_cache_clear(tmp);
        g_slice_free(PyGIArgCache, tmp); 
    }

    g_slice_free(PyGIFunctionCache, cache);
}

/* cache generation */
static inline PyGIFunctionCache *
_function_cache_init(GIFunctionInfo *function_info)
{
    PyGIFunctionCache *fc;
    GIFunctionInfoFlags flags;

    fc = g_slice_new0(PyGIFunctionCache);
    flags = g_function_info_get_flags(function_info);
    fc->is_method = flags & GI_FUNCTION_IS_METHOD;
    fc->is_constructor = flags & GI_FUNCTION_IS_CONSTRUCTOR;
    fc->n_args = g_callable_info_get_n_args ( (GICallableInfo *) function_info);
    fc->args_cache = g_slice_alloc0(fc->n_args * sizeof(PyGIArgInfo *));

    return fc;
}

/* process in args */

static inline boolean
_arg_cache_generate_metadata_in_void(PyGIArgCache *arg_cache,
                                     PyGIFunctionCache *function_cache,
                                     GIArgInfo *arg_info)
{
     arg_cache->in_marshaler = _pygi_marshal_in_void;

     return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_boolean(PyGIArgCache *arg_cache,
                                        PyGIFunctionCache *function_cache,
                                        GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_boolean; 
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_int8(PyGIArgCache *arg_cache,
                                     PyGIFunctionCache *function_cache,
                                     GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_int8;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_uint8(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_uint8;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_int16(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_int16;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_uint16(PyGIArgCache *arg_cache,
                                       PyGIFunctionCache *function_cache,
                                       GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_uint16;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_int32(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_int32;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_uint32(PyGIArgCache *arg_cache,
                                       PyGIFunctionCache *function_cache,
                                       GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_uint32;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_int64(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_int64;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_uint64(PyGIArgCache *arg_cache,
                                       PyGIFunctionCache *function_cache,
                                       GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_uint64;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_float(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_float;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_double(PyGIArgCache *arg_cache,
                                       PyGIFunctionCache *function_cache,
                                       GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_double;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_unichar(PyGIArgCache *arg_cache,
                                        PyGIFunctionCache *function_cache,
                                        GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_unichar;

    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_gtype(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_gtype;
    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_utf8(PyGIArgCache *arg_cache,
                                     PyGIFunctionCache *function_cache,
                                     GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_utf8;
    if (arg_cache->transfer == GI_TRANSFER_NOTHING)
        arg_cache->cleanup = g_free;

    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_filename(PyGIArgCache *arg_cache,
                                         PyGIFunctionCache *function_cache,
                                         GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_filename;
    if (arg_cache->transfer == GI_TRANSFER_NOTHING)
        arg_cache->cleanup = g_free;

    return TRUE;
}

static inline boolean
_arg_cache_generate_metadata_in_array(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_array;
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return FALSE;
}

static inline boolean
_arg_cache_generate_metadata_in_interface(PyGIArgCache *arg_cache,
                                          PyGIFunctionCache *function_cache,
                                          GIArgInfo *arg_info)
{
    /* TODO: Switch on GI_INFO_TYPE_ to determine caching */
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return FALSE;
}

static inline boolean
_arg_cache_generate_metadata_in_glist(PyGIArgCache *arg_cache,
                                     PyGIFunctionCache *function_cache,
                                     GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_glist;
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return FALSE;
}

static inline boolean
_arg_cache_generate_metadata_in_gslist(PyGIArgCache *arg_cache,
                                     PyGIFunctionCache *function_cache,
                                     GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_gslist;
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return FALSE;
}

static inline boolean
_arg_cache_generate_metadata_in_ghash(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_ghash;
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return FALSE;
}

static inline boolean
_arg_cache_generate_metadata_in_error(PyGIArgCache *arg_cache,
                                      PyGIFunctionCache *function_cache,
                                      GIArgInfo *arg_info)
{
    arg_cache->in_marshaler = _pygi_marshal_in_error;
    PyErr_Format(PyExc_NotImplementedError,
                 "Caching for this type is not fully implemented yet");
    return FALSE;
}

static inline boolean
_arg_cache_generate_metadata_in(PyGIArgCache *arg_cache,
                                PyGIFunctionCache *function_cache,
                                GIArgInfo *arg_info,
                                GITypeTag type_tag)
{
    gboolean success = True;

    arg_cache->c_arg_index = i + function_cache->is_method;
    arg_cache->py_arg_index =
        function_info->n_in_args + function_cache->is_method;

    function_info->n_in_args++;
    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
           success = _arg_cache_generate_metadata_in_void(arg_cache,
                                                          function_cache,
                                                          arg_info);
           break;
       case GI_TYPE_TAG_BOOLEAN:
           success = _arg_cache_generate_metadata_in_boolean(arg_cache,
                                                             function_cache,
                                                             arg_info);
           break;
       case GI_TYPE_TAG_INT8:
           success = _arg_cache_generate_metadata_in_int8(arg_cache,
                                                          function_cache,
                                                          arg_info);
           break;
       case GI_TYPE_TAG_UINT8:
           success = _arg_cache_generate_metadata_in_uint8(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_INT16:
           success = _arg_cache_generate_metadata_in_uint16(arg_cache,
                                                            function_cache,
                                                            arg_info);
           break;
       case GI_TYPE_TAG_UINT16:
           success = _arg_cache_generate_metadata_in_uint16(arg_cache,
                                                            function_cache,
                                                            arg_info);
           break;
       case GI_TYPE_TAG_INT32:
           success = _arg_cache_generate_metadata_in_int32(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_UINT32:
           success = _arg_cache_generate_metadata_in_uint32(arg_cache,
                                                            function_cache,
                                                            arg_info);
           break;
       case GI_TYPE_TAG_INT64:
           success = _arg_cache_generate_metadata_in_int64(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_UINT64:
           success = _arg_cache_generate_metadata_in_uint64(arg_cache,
                                                            function_cache,
                                                            arg_info);
           break;
       case GI_TYPE_TAG_FLOAT:
           success = _arg_cache_generate_metadata_in_float(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_DOUBLE:
           success = _arg_cache_generate_metadata_in_double(arg_cache,
                                                            function_cache,
                                                            arg_info);
           break;
       case GI_TYPE_TAG_UNICHAR:
           success = _arg_cache_generate_metadata_in_unichar(arg_cache,
                                                             function_cache,
                                                             arg_info);
           break;
       case GI_TYPE_TAG_GTYPE:
           success = _arg_cache_generate_metadata_in_gtype(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_UTF8:
           success = _arg_cache_generate_metadata_in_utf8(arg_cache,
                                                          function_cache,
                                                          arg_info);
           break;
       case GI_TYPE_TAG_FILENAME:
           success = _arg_cache_generate_metadata_in_filename(arg_cache,
                                                              function_cache,
                                                              arg_info);
           break;
       case GI_TYPE_TAG_ARRAY:
           success = _arg_cache_generate_metadata_in_array(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_INTERFACE:
           success = _arg_cache_generate_metadata_in_interface(arg_cache,
                                                               function_cache,
                                                               arg_info);
           break;
       case GI_TYPE_TAG_GLIST:
           success = _arg_cache_generate_metadata_in_glist(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_GSLIST:
           success = _arg_cache_generate_metadata_in_gslist(arg_cache,
                                                            function_cache,
                                                            arg_info);
           break;
       case GI_TYPE_TAG_GHASH:
           success = _arg_cache_generate_metadata_in_ghash(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
       case GI_TYPE_TAG_ERROR:
           success = _arg_cache_generate_metadata_in_error(arg_cache,
                                                           function_cache,
                                                           arg_info);
           break;
    }

    function_cache->in_args = 
        g_slist_append(function_cache->in_args, arg_cache);

    return success;
}

static inline boolean
_args_cache_generate(GIFunctionInfo *function_info, 
                     PyGIFunctionCache *function_cache)
{
    for (i = 0; i < function_cache->n_args; i++) {
        PyGIArgCache *arg_cache;
        GIArgInfo *arg_info;

        /* must be an aux arg filled in by its owner so skip */
        if (function_cache->args_cache[i] != NULL)
            continue;

        arg_info = 
            g_callable_info_get_arg ( (GICallableInfo *) function_info, i);

        arg_cache = function_cache->args_cache[i] = g_slice_new0(PyGIArgCache);
        arg_cache->direction = g_arg_info_get_direction (arg_info);
        type_info = g_base_info_get_type ( (GIBaseInfo *) arg_info);
        type_tag = g_type_info_get_tag (type_info);

        switch(direction) {
            case GI_DIRECTION_IN:
                _arg_cache_generate_metadata_in(arg_cache,
                                                function_cache,
                                                arg_info,
                                                type_tag);

                break;

            case GI_DIRECTION_OUT:
                function_info->n_out_args++;
 		    switch (type_tag) {
                    case GI_TYPE_TAG_...:

                        ac->out_marshaler = <type marshaling function pointer>
                        ac->cleanup = <type cleanup function pointer>
                        fc->out_args = g_slist_append(fc->out_args, ac);
                        break;
           }
        }
    }
}

PyGIFunctionCache *
_pygi_function_cache_new (GIFunctionInfo *function_info)
{
    PyGIFunction *fc = _init_function_cache(function_info);
    if (!_args_cache_generate(function_info, fc))
        goto err;

err:
    _pygi_function_cache_free(fc);
    return NULL; 
}
