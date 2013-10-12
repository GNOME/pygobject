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

#include <girepository.h>

#include "pygi-info.h"
#include "pygi-cache.h"
#include "pygi-marshal-to-py.h"
#include "pygi-marshal-from-py.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-type.h"
#include "pygi-hashtable.h"
#include "pygi-basictype.h"
#include "pygi-list.h"
#include "pygi-array.h"
#include "pygi-error.h"


PyGIArgCache * _arg_cache_new_for_interface (GIInterfaceInfo *iface_info,
                                             GITypeInfo *type_info,
                                             GIArgInfo *arg_info,
                                             GITransfer transfer,
                                             PyGIDirection direction,
                                             /* will be removed */
                                             PyGICallableCache *callable_cache);


/* _arg_info_default_value
 * info:
 * arg: (out): GIArgument to fill in with default value.
 *
 * This is currently a place holder API which only supports "allow-none" pointer args.
 * Once defaults are part of the GI API, we can replace this with: g_arg_info_default_value
 * https://bugzilla.gnome.org/show_bug.cgi?id=558620
 *
 * Returns: TRUE if the given argument supports a default value and was filled in.
 */
static gboolean
_arg_info_default_value (GIArgInfo *info, GIArgument *arg)
{
    if (g_arg_info_may_be_null (info)) {
        arg->v_pointer = NULL;
        return TRUE;
    }
    return FALSE;
}

gboolean
pygi_arg_base_setup (PyGIArgCache *arg_cache,
                     GITypeInfo   *type_info,
                     GIArgInfo    *arg_info,  /* may be NULL for return arguments */
                     GITransfer    transfer,
                     PyGIDirection direction)
{
    arg_cache->direction = direction;
    arg_cache->transfer = transfer;
    arg_cache->py_arg_index = -1;
    arg_cache->c_arg_index = -1;

    if (type_info != NULL) {
        arg_cache->is_pointer = g_type_info_is_pointer (type_info);
        arg_cache->type_tag = g_type_info_get_tag (type_info);
        g_base_info_ref ( (GIBaseInfo *) type_info);
        arg_cache->type_info = type_info;
    }

    if (arg_info != NULL) {
        if (!arg_cache->has_default) {
            /* It is possible has_default was set somewhere else */
            arg_cache->has_default = _arg_info_default_value (arg_info,
                                                              &arg_cache->default_value);
        }
        arg_cache->arg_name = g_base_info_get_name ((GIBaseInfo *) arg_info);
        arg_cache->allow_none = g_arg_info_may_be_null (arg_info);

        if (arg_cache->type_tag == GI_TYPE_TAG_INTERFACE || arg_cache->type_tag == GI_TYPE_TAG_ARRAY)
            arg_cache->is_caller_allocates = g_arg_info_is_caller_allocates (arg_info);
        else
            arg_cache->is_caller_allocates = FALSE;
    }
    return TRUE;
}


gboolean
pygi_arg_interface_setup (PyGIInterfaceCache *iface_cache,
                          GITypeInfo         *type_info,
                          GIArgInfo          *arg_info,    /* may be NULL for return arguments */
                          GITransfer          transfer,
                          PyGIDirection       direction,
                          GIInterfaceInfo    *iface_info)
{
    if (!pygi_arg_base_setup ((PyGIArgCache *)iface_cache,
                              type_info,
                              arg_info,
                              transfer,
                              direction)) {
        return FALSE;
    }

    g_base_info_ref ( (GIBaseInfo *)iface_info);
    iface_cache->interface_info = iface_info;
    iface_cache->arg_cache.type_tag = GI_TYPE_TAG_INTERFACE;

    return TRUE;
}


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
    if (cache == NULL)
        return;

    g_slist_free (cache->to_py_args);
    g_slist_free (cache->arg_name_list);
    g_hash_table_destroy (cache->arg_name_hash);
    g_ptr_array_unref (cache->args_cache);

    if (cache->return_cache != NULL)
        _pygi_arg_cache_free (cache->return_cache);

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

gboolean
pygi_arg_sequence_setup (PyGISequenceCache  *sc,
                         GITypeInfo         *type_info,
                         GIArgInfo          *arg_info,    /* may be NULL for return arguments */
                         GITransfer          transfer,
                         PyGIDirection       direction)
{
    GITypeInfo *item_type_info;
    GITransfer item_transfer;

    if (!pygi_arg_base_setup ((PyGIArgCache *)sc,
                              type_info,
                              arg_info,
                              transfer,
                              direction)) {
        return FALSE;
    }

    sc->arg_cache.destroy_notify = (GDestroyNotify)_sequence_cache_free_func;
    item_type_info = g_type_info_get_param_type (type_info, 0);
    item_transfer =
        transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

    sc->item_cache = _arg_cache_new (item_type_info,
                                     NULL,
                                     item_transfer,
                                     direction,
                                     0, 0,
                                     NULL);

    g_base_info_unref ( (GIBaseInfo *)item_type_info);

    if (sc->item_cache == NULL) {
        return FALSE;
    }

    return TRUE;
}

static PyGICallbackCache *
_callback_cache_new (GIArgInfo *arg_info,
                     GIInterfaceInfo *iface_info,
                     gssize child_offset)
{
   PyGICallbackCache *cc;

   cc = g_slice_new0 (PyGICallbackCache);
   ( (PyGIArgCache *)cc)->destroy_notify = (GDestroyNotify)_callback_cache_free_func;

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

PyGIArgCache *
_arg_cache_alloc (void)
{
    return g_slice_new0 (PyGIArgCache);
}

static void
_arg_cache_from_py_interface_union_setup (PyGIArgCache *arg_cache,
                                          GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_struct_cache_adapter;
}

static void
_arg_cache_to_py_interface_union_setup (PyGIArgCache *arg_cache,
                                        GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_struct_cache_adapter;
}

static void
_arg_cache_from_py_interface_struct_setup (PyGIArgCache *arg_cache,
                                           GIInterfaceInfo *iface_info,
                                           GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign ( (GIStructInfo*)iface_info);
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_struct_cache_adapter;

    if (iface_cache->g_type == G_TYPE_VALUE)
        arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_interface_struct_gvalue;
    else if (iface_cache->is_foreign)
        arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_interface_struct_foreign;
}

static void
_arg_cache_to_py_interface_struct_setup (PyGIArgCache *arg_cache,
                                         GIInterfaceInfo *iface_info,
                                         GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    iface_cache->is_foreign = g_struct_info_is_foreign ( (GIStructInfo*)iface_info);
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_struct_cache_adapter;

    if (iface_cache->is_foreign)
        arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_interface_struct_foreign;
}

static void
_arg_cache_from_py_interface_object_setup (PyGIArgCache *arg_cache,
                                           GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_object;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_interface_object;
}

static void
_arg_cache_to_py_interface_object_setup (PyGIArgCache *arg_cache,
                                         GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_object_cache_adapter;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_interface_object;
}

static void
_arg_cache_from_py_interface_callback_setup (PyGIArgCache *arg_cache,
                                             PyGICallableCache *callable_cache)
{
    PyGICallbackCache *callback_cache = (PyGICallbackCache *)arg_cache;
    if (callback_cache->user_data_index >= 0) {
        PyGIArgCache *user_data_arg_cache = _arg_cache_alloc ();
        user_data_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD_WITH_PYARG;
        user_data_arg_cache->direction = PYGI_DIRECTION_FROM_PYTHON;
        user_data_arg_cache->has_default = TRUE; /* always allow user data with a NULL default. */
        _pygi_callable_cache_set_arg (callable_cache, callback_cache->user_data_index,
                                      user_data_arg_cache);
    }

    if (callback_cache->destroy_notify_index >= 0) {
        PyGIArgCache *destroy_arg_cache = _arg_cache_alloc ();
        destroy_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        destroy_arg_cache->direction = PYGI_DIRECTION_FROM_PYTHON;
        _pygi_callable_cache_set_arg (callable_cache, callback_cache->destroy_notify_index,
                                      destroy_arg_cache);
    }
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_callback;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_interface_callback;
}

static void
_arg_cache_to_py_interface_callback_setup (void)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Callback returns are not supported");
}

static void
_arg_cache_from_py_interface_enum_setup (PyGIArgCache *arg_cache,
                                         GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_enum;
}

static void
_arg_cache_to_py_interface_enum_setup (PyGIArgCache *arg_cache,
                                       GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_enum;
}

static void
_arg_cache_from_py_interface_flags_setup (PyGIArgCache *arg_cache,
                                          GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_flags;
}

static void
_arg_cache_to_py_interface_flags_setup (PyGIArgCache *arg_cache,
                                        GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_flags;
}


PyGIArgCache *
_arg_cache_new_for_interface (GIInterfaceInfo   *iface_info,
                              GITypeInfo        *type_info,
                              GIArgInfo         *arg_info,
                              GITransfer         transfer,
                              PyGIDirection      direction,
                              PyGICallableCache *callable_cache)
{
    PyGIArgCache *arg_cache = NULL;
    gssize child_offset = 0;
    GIInfoType info_type;

    if (callable_cache != NULL)
        child_offset =
            (callable_cache->function_type == PYGI_FUNCTION_TYPE_METHOD ||
                 callable_cache->function_type == PYGI_FUNCTION_TYPE_VFUNC) ? 1: 0;

    info_type = g_base_info_get_type ( (GIBaseInfo *)iface_info);

    /* Callbacks are special cased */
    if (info_type == GI_INFO_TYPE_CALLBACK) {
        PyGICallbackCache *callback_cache;

        if (direction & PYGI_DIRECTION_TO_PYTHON) {
            _arg_cache_to_py_interface_callback_setup ();
            return NULL;
        }

        callback_cache =
            _callback_cache_new (arg_info,
                                 iface_info,
                                 child_offset);

        arg_cache = (PyGIArgCache *)callback_cache;
        if (arg_cache == NULL)
            return NULL;

        pygi_arg_base_setup (arg_cache,
                             type_info,
                             arg_info,
                             transfer,
                             direction);

        if (direction & PYGI_DIRECTION_FROM_PYTHON)
            _arg_cache_from_py_interface_callback_setup (arg_cache, callable_cache);

        return arg_cache;

    }

    arg_cache = (PyGIArgCache *)_interface_cache_new (iface_info);
    if (arg_cache == NULL)
        return NULL;

    switch (info_type) {
        case GI_INFO_TYPE_UNION:
            if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_interface_union_setup (arg_cache, transfer);

            if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_interface_union_setup (arg_cache, transfer);

            break;
        case GI_INFO_TYPE_BOXED:
        case GI_INFO_TYPE_STRUCT:
            if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_interface_struct_setup (arg_cache,
                                                          iface_info,
                                                          transfer);

            if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_interface_struct_setup (arg_cache,
                                                        iface_info,
                                                        transfer);
            break;
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_interface_object_setup (arg_cache, transfer);

            if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_interface_object_setup (arg_cache, transfer);

            break;
        case GI_INFO_TYPE_ENUM:
            if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_interface_enum_setup (arg_cache, transfer);

            if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_interface_enum_setup (arg_cache, transfer);

            break;
        case GI_INFO_TYPE_FLAGS:
            if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_interface_flags_setup (arg_cache, transfer);

            if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_interface_flags_setup (arg_cache, transfer);

            break;
        default:
            g_assert_not_reached ();
    }

    pygi_arg_interface_setup ((PyGIInterfaceCache *)arg_cache,
                              type_info,
                              arg_info,
                              transfer,
                              direction,
                              iface_info);

    return arg_cache;
}

PyGIArgCache *
_arg_cache_new (GITypeInfo *type_info,
                GIArgInfo *arg_info,     /* may be null */
                GITransfer transfer,
                PyGIDirection direction,
                gssize c_arg_index,
                gssize py_arg_index,
                PyGICallableCache *callable_cache)
{
    PyGIArgCache *arg_cache = NULL;
    GITypeTag type_tag;

    type_tag = g_type_info_get_tag (type_info);

    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
       case GI_TYPE_TAG_BOOLEAN:
       case GI_TYPE_TAG_INT8:
       case GI_TYPE_TAG_UINT8:
       case GI_TYPE_TAG_INT16:
       case GI_TYPE_TAG_UINT16:
       case GI_TYPE_TAG_INT32:
       case GI_TYPE_TAG_UINT32:
       case GI_TYPE_TAG_INT64:
       case GI_TYPE_TAG_UINT64:
       case GI_TYPE_TAG_FLOAT:
       case GI_TYPE_TAG_DOUBLE:
       case GI_TYPE_TAG_UNICHAR:
       case GI_TYPE_TAG_GTYPE:
       case GI_TYPE_TAG_UTF8:
       case GI_TYPE_TAG_FILENAME:
           arg_cache = pygi_arg_basic_type_new_from_info (type_info,
                                                          arg_info,
                                                          transfer,
                                                          direction);
           break;

       case GI_TYPE_TAG_ARRAY:
           {
               arg_cache = pygi_arg_garray_new_from_info (type_info,
                                                          arg_info,
                                                          transfer,
                                                          direction);
               if (arg_cache == NULL)
                   return NULL;

               pygi_arg_garray_len_arg_setup (arg_cache,
                                              type_info,
                                              callable_cache,
                                              direction,
                                              c_arg_index,
                                              &py_arg_index);
           }
           break;

       case GI_TYPE_TAG_GLIST:
           arg_cache = pygi_arg_glist_new_from_info (type_info,
                                                     arg_info,
                                                     transfer,
                                                     direction);
           break;

       case GI_TYPE_TAG_GSLIST:
           arg_cache = pygi_arg_gslist_new_from_info (type_info,
                                                      arg_info,
                                                      transfer,
                                                      direction);
           break;

       case GI_TYPE_TAG_GHASH:
           arg_cache = pygi_arg_hash_table_new_from_info (type_info,
                                                          arg_info,
                                                          transfer,
                                                          direction);
           break;

       case GI_TYPE_TAG_INTERFACE:
           {
               GIInterfaceInfo *interface_info = g_type_info_get_interface (type_info);
               arg_cache = _arg_cache_new_for_interface (interface_info,
                                                         type_info,
                                                         arg_info,
                                                         transfer,
                                                         direction,
                                                         callable_cache);

               g_base_info_unref ( (GIBaseInfo *)interface_info);
           }
           break;

       case GI_TYPE_TAG_ERROR:
           arg_cache = pygi_arg_gerror_new_from_info (type_info,
                                                      arg_info,
                                                      transfer,
                                                      direction);
           break;
    }

    if (arg_cache != NULL) {
        arg_cache->py_arg_index = py_arg_index;
        arg_cache->c_arg_index = c_arg_index;
    }

    return arg_cache;
}

static PyGIDirection
_pygi_get_direction (PyGICallableCache *callable_cache, GIDirection gi_direction)
{
    /* For vfuncs and callbacks our marshalling directions are reversed */
    if (gi_direction == GI_DIRECTION_INOUT) {
        return PYGI_DIRECTION_BIDIRECTIONAL;
    } else if (gi_direction == GI_DIRECTION_IN) {
        if (callable_cache->function_type == PYGI_FUNCTION_TYPE_CALLBACK)
            return PYGI_DIRECTION_TO_PYTHON;
        return PYGI_DIRECTION_FROM_PYTHON;
    } else {
        if (callable_cache->function_type == PYGI_FUNCTION_TYPE_CALLBACK)
            return PYGI_DIRECTION_FROM_PYTHON;
        return PYGI_DIRECTION_TO_PYTHON;
    }
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
    PyGIDirection return_direction;

    /* Return arguments are always considered out */
    return_direction = _pygi_get_direction (callable_cache, GI_DIRECTION_OUT);

    /* cache the return arg */
    return_info =
        g_callable_info_get_return_type (callable_info);
    return_transfer =
        g_callable_info_get_caller_owns (callable_info);
    return_cache =
        _arg_cache_new (return_info,
                        NULL,
                        return_transfer,
                        return_direction,
                        -1,
                        -1,
                        callable_cache);
    if (return_cache == NULL)
        return FALSE;

    return_cache->is_skipped = g_callable_info_skip_return (callable_info);
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
                                          NULL,
                                          NULL,
                                          GI_TRANSFER_NOTHING,
                                          PYGI_DIRECTION_FROM_PYTHON,
                                          callable_cache);

        g_base_info_unref ( (GIBaseInfo *)interface_info);

        if (instance_cache == NULL)
            return FALSE;

        /* Because we are not supplied a GITypeInfo for instance arguments,
         * assume some defaults. */
        instance_cache->is_pointer = TRUE;
        instance_cache->py_arg_index = 0;
        instance_cache->c_arg_index = 0;

        _pygi_callable_cache_set_arg (callable_cache, arg_index, instance_cache);

        arg_index++;
        callable_cache->n_from_py_args++;
        callable_cache->n_py_args++;
    }


    for (i=0; arg_index < _pygi_callable_cache_args_len (callable_cache); arg_index++, i++) {
        PyGIArgCache *arg_cache = NULL;
        GIArgInfo *arg_info;
        PyGIDirection direction;

        arg_info = g_callable_info_get_arg (callable_info, i);

        if (g_arg_info_get_closure (arg_info) == i) {

            arg_cache = _arg_cache_alloc ();
            _pygi_callable_cache_set_arg (callable_cache, arg_index, arg_cache);

            direction = PYGI_DIRECTION_FROM_PYTHON;
            arg_cache->direction = direction;
            arg_cache->meta_type = PYGI_META_ARG_TYPE_CLOSURE;
            arg_cache->c_arg_index = i;

            callable_cache->n_from_py_args++;

        } else {
            GITypeInfo *type_info;

            direction = _pygi_get_direction (callable_cache,
                                             g_arg_info_get_direction (arg_info));
            type_info = g_arg_info_get_type (arg_info);

            /* must be an child arg filled in by its owner
             * and continue
             * fill in it's c_arg_index, add to the in count
             */
            arg_cache = _pygi_callable_cache_get_arg (callable_cache, arg_index);
            if (arg_cache != NULL) {
                if (arg_cache->meta_type == PYGI_META_ARG_TYPE_CHILD_WITH_PYARG) {
                    arg_cache->py_arg_index = callable_cache->n_py_args;
                    callable_cache->n_py_args++;
                }

                if (direction & PYGI_DIRECTION_FROM_PYTHON) {
                    arg_cache->c_arg_index = callable_cache->n_from_py_args;
                    callable_cache->n_from_py_args++;
                }

                if (direction & PYGI_DIRECTION_TO_PYTHON) {
                    callable_cache->n_to_py_args++;
                }

                arg_cache->type_tag = g_type_info_get_tag (type_info);

            } else {
                GITransfer transfer;
                gssize py_arg_index = -1;
                transfer = g_arg_info_get_ownership_transfer (arg_info);

                if (direction & PYGI_DIRECTION_FROM_PYTHON) {
                    py_arg_index = callable_cache->n_py_args;
                    callable_cache->n_from_py_args++;
                    callable_cache->n_py_args++;
                }

                arg_cache =
                    _arg_cache_new (type_info,
                                    arg_info,
                                    transfer,
                                    direction,
                                    arg_index,
                                    py_arg_index,
                                    callable_cache);

                if (arg_cache == NULL) {
                    g_base_info_unref( (GIBaseInfo *)type_info);
                    g_base_info_unref( (GIBaseInfo *)arg_info);
                    return FALSE;
                }


                if (direction & PYGI_DIRECTION_TO_PYTHON) {
                    callable_cache->n_to_py_args++;

                    callable_cache->to_py_args =
                        g_slist_append (callable_cache->to_py_args, arg_cache);
                }

                _pygi_callable_cache_set_arg (callable_cache, arg_index, arg_cache);
            }

            g_base_info_unref (type_info);
        }

        /* Ensure arguments always have a name when available */
        arg_cache->arg_name = g_base_info_get_name ((GIBaseInfo *) arg_info);

        g_base_info_unref ( (GIBaseInfo *)arg_info);

    }

    if (callable_cache->arg_name_hash == NULL) {
        callable_cache->arg_name_hash = g_hash_table_new (g_str_hash, g_str_equal);
    } else {
        g_hash_table_remove_all (callable_cache->arg_name_hash);
    }
    callable_cache->n_py_required_args = 0;
    callable_cache->user_data_varargs_index = -1;

    gssize last_explicit_arg_index = -1;

    /* Reverse loop through all the arguments to setup arg_name_list/hash
     * and find the number of required arguments */
    for (i=((gssize)_pygi_callable_cache_args_len (callable_cache))-1; i >= 0; i--) {
        PyGIArgCache *arg_cache = _pygi_callable_cache_get_arg (callable_cache, i);

        if (arg_cache->meta_type != PYGI_META_ARG_TYPE_CHILD &&
                arg_cache->meta_type != PYGI_META_ARG_TYPE_CLOSURE &&
                arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {

            /* Setup arg_name_list and arg_name_hash */
            gpointer arg_name = (gpointer)arg_cache->arg_name;
            callable_cache->arg_name_list = g_slist_prepend (callable_cache->arg_name_list,
                                                             arg_name);
            if (arg_name != NULL) {
                g_hash_table_insert (callable_cache->arg_name_hash,
                                     arg_name,
                                     GINT_TO_POINTER(i));
            }

            /* The first tail argument without a default will force all the preceding
             * argument defaults off. This limits support of default args to the
             * tail of an args list.
             */
            if (callable_cache->n_py_required_args > 0) {
                arg_cache->has_default = FALSE;
                callable_cache->n_py_required_args += 1;
            } else if (!arg_cache->has_default) {
                callable_cache->n_py_required_args += 1;
            }

            if (last_explicit_arg_index == -1) {
                last_explicit_arg_index = i;

                /* If the last "from python" argument in the args list is a child
                 * with pyarg (currently only callback user_data). Set it to eat
                 * variable args in the callable cache.
                 */
                if (arg_cache->meta_type == PYGI_META_ARG_TYPE_CHILD_WITH_PYARG)
                    callable_cache->user_data_varargs_index = i;
            }
        }
    }

    return TRUE;
}

PyGICallableCache *
_pygi_callable_cache_new (GICallableInfo *callable_info, gboolean is_ccallback)
{
    gint n_args;
    PyGICallableCache *cache;
    GIInfoType type = g_base_info_get_type ( (GIBaseInfo *)callable_info);

    cache = g_slice_new0 (PyGICallableCache);

    if (cache == NULL)
        return NULL;

    cache->name = g_base_info_get_name ((GIBaseInfo *)callable_info);

    if (g_base_info_is_deprecated (callable_info)) {
        const gchar *deprecated = g_base_info_get_attribute (callable_info, "deprecated");
        gchar *warning;
        if (deprecated != NULL)
            warning = g_strdup_printf ("%s.%s is deprecated: %s",
                                       g_base_info_get_namespace (callable_info), cache->name,
                                       deprecated);
        else
            warning = g_strdup_printf ("%s.%s is deprecated",
                                       g_base_info_get_namespace (callable_info), cache->name);
        PyErr_WarnEx(PyExc_DeprecationWarning, warning, 0);
        g_free (warning);
    }

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
        if (is_ccallback)
            cache->function_type = PYGI_FUNCTION_TYPE_CCALLBACK;
        else
            cache->function_type = PYGI_FUNCTION_TYPE_CALLBACK;
    } else {
        cache->function_type = PYGI_FUNCTION_TYPE_METHOD;
    }

    n_args = g_callable_info_get_n_args (callable_info);

    /* if we are a method or vfunc make sure the instance parameter is counted */
    if (cache->function_type == PYGI_FUNCTION_TYPE_METHOD ||
            cache->function_type == PYGI_FUNCTION_TYPE_VFUNC)
        n_args++;

    if (n_args >= 0) {
        cache->args_cache = g_ptr_array_new_full (n_args, (GDestroyNotify) _pygi_arg_cache_free);
        g_ptr_array_set_size (cache->args_cache, n_args);
    }

    if (!_args_cache_generate (callable_info, cache))
        goto err;

    return cache;
err:
    _pygi_callable_cache_free (cache);
    return NULL;
}
