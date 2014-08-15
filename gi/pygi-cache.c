/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2013 Simon Feltman <sfeltman@gnome.org>
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

#include <girepository.h>

#include "pyglib.h"
#include "pygi-info.h"
#include "pygi-cache.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-type.h"
#include "pygi-hashtable.h"
#include "pygi-basictype.h"
#include "pygi-list.h"
#include "pygi-array.h"
#include "pygi-closure.h"
#include "pygi-error.h"
#include "pygi-object.h"
#include "pygi-struct-marshal.h"
#include "pygi-enum-marshal.h"


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

/* pygi_arg_base_setup:
 * arg_cache: argument cache to initialize
 * type_info: source for type related attributes to cache
 * arg_info: (allow-none): source for argument related attributes to cache
 * transfer: transfer mode to store in the argument cache
 * direction: marshaling direction to store in the cache
 *
 * Initializer for PyGIArgCache
 *
 * Returns: TRUE on success and FALSE on failure
 */
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

void
pygi_arg_cache_free (PyGIArgCache *cache)
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

/* PyGIInterfaceCache */

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

/* pygi_arg_interface_setup:
 * arg_cache: argument cache to initialize
 * type_info: source for type related attributes to cache
 * arg_info: (allow-none): source for argument related attributes to cache
 * transfer: transfer mode to store in the argument cache
 * direction: marshaling direction to store in the cache
 * iface_info: interface info to cache
 *
 * Initializer for PyGIInterfaceCache
 *
 * Returns: TRUE on success and FALSE on failure
 */
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

    ( (PyGIArgCache *)iface_cache)->destroy_notify = (GDestroyNotify)_interface_cache_free_func;

    g_base_info_ref ( (GIBaseInfo *)iface_info);
    iface_cache->interface_info = iface_info;
    iface_cache->arg_cache.type_tag = GI_TYPE_TAG_INTERFACE;
    iface_cache->type_name = _pygi_g_base_info_get_fullname (iface_info);
    iface_cache->g_type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *)iface_info);
    iface_cache->py_type = _pygi_type_import_by_gi_info ( (GIBaseInfo *) iface_info);

    if (iface_cache->py_type == NULL) {
        return FALSE;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_interface_new_from_info (GITypeInfo         *type_info,
                                  GIArgInfo          *arg_info,    /* may be NULL for return arguments */
                                  GITransfer          transfer,
                                  PyGIDirection       direction,
                                  GIInterfaceInfo    *iface_info)
{
    PyGIInterfaceCache *ic;

    ic = g_slice_new0 (PyGIInterfaceCache);
    if (!pygi_arg_interface_setup (ic,
                                   type_info,
                                   arg_info,
                                   transfer,
                                   direction,
                                   iface_info)) {
        pygi_arg_cache_free ((PyGIArgCache *)ic);
        return NULL;
    }

    return (PyGIArgCache *)ic;
}

/* PyGISequenceCache */

static void
_sequence_cache_free_func (PyGISequenceCache *cache)
{
    if (cache != NULL) {
        pygi_arg_cache_free (cache->item_cache);
        g_slice_free (PyGISequenceCache, cache);
    }
}

/* pygi_arg_sequence_setup:
 * sc: sequence cache to initialize
 * type_info: source for type related attributes to cache
 * arg_info: (allow-none): source for argument related attributes to cache
 * transfer: transfer mode to store in the argument cache
 * direction: marshaling direction to store in the cache
 * iface_info: interface info to cache
 *
 * Initializer for PyGISequenceCache used for holding list and array argument
 * caches.
 *
 * Returns: TRUE on success and FALSE on failure
 */
gboolean
pygi_arg_sequence_setup (PyGISequenceCache  *sc,
                         GITypeInfo         *type_info,
                         GIArgInfo          *arg_info,    /* may be NULL for return arguments */
                         GITransfer          transfer,
                         PyGIDirection       direction,
                         PyGICallableCache  *callable_cache)
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

    sc->item_cache = pygi_arg_cache_new (item_type_info,
                                         NULL,
                                         item_transfer,
                                         direction,
                                         callable_cache,
                                         0, 0);

    g_base_info_unref ( (GIBaseInfo *)item_type_info);

    if (sc->item_cache == NULL) {
        return FALSE;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_cache_alloc (void)
{
    return g_slice_new0 (PyGIArgCache);
}

static PyGIArgCache *
_arg_cache_new_for_interface (GIInterfaceInfo   *iface_info,
                              GITypeInfo        *type_info,
                              GIArgInfo         *arg_info,
                              GITransfer         transfer,
                              PyGIDirection      direction,
                              PyGICallableCache *callable_cache)
{
    GIInfoType info_type;

    info_type = g_base_info_get_type ( (GIBaseInfo *)iface_info);

    switch (info_type) {
        case GI_INFO_TYPE_CALLBACK:
            return pygi_arg_callback_new_from_info (type_info,
                                                    arg_info,
                                                    transfer,
                                                    direction,
                                                    iface_info,
                                                    callable_cache);
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            return pygi_arg_gobject_new_from_info (type_info,
                                                   arg_info,
                                                   transfer,
                                                   direction,
                                                   iface_info,
                                                   callable_cache);
        case GI_INFO_TYPE_BOXED:
        case GI_INFO_TYPE_STRUCT:
        case GI_INFO_TYPE_UNION:
            return pygi_arg_struct_new_from_info (type_info,
                                                  arg_info,
                                                  transfer,
                                                  direction,
                                                  iface_info);
        case GI_INFO_TYPE_ENUM:
            return pygi_arg_enum_new_from_info (type_info,
                                                arg_info,
                                                transfer,
                                                direction,
                                                iface_info);
        case GI_INFO_TYPE_FLAGS:
            return pygi_arg_flags_new_from_info (type_info,
                                                 arg_info,
                                                 transfer,
                                                 direction,
                                                 iface_info);
        default:
            g_assert_not_reached ();
    }

    return NULL;
}

PyGIArgCache *
pygi_arg_cache_new (GITypeInfo *type_info,
                    GIArgInfo *arg_info,     /* may be null */
                    GITransfer transfer,
                    PyGIDirection direction,
                    PyGICallableCache *callable_cache,
                    gssize c_arg_index,
                    gssize py_arg_index)
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
                                                          direction,
                                                          callable_cache);
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
                                                     direction,
                                                     callable_cache);
           break;

       case GI_TYPE_TAG_GSLIST:
           arg_cache = pygi_arg_gslist_new_from_info (type_info,
                                                      arg_info,
                                                      transfer,
                                                      direction,
                                                      callable_cache);
           break;

       case GI_TYPE_TAG_GHASH:
           arg_cache = pygi_arg_hash_table_new_from_info (type_info,
                                                          arg_info,
                                                          transfer,
                                                          direction,
                                                          callable_cache);
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

/* PyGICallableCache */

static PyGIDirection
_pygi_get_direction (PyGICallableCache *callable_cache, GIDirection gi_direction)
{
    /* For vfuncs and callbacks our marshalling directions are reversed */
    if (gi_direction == GI_DIRECTION_INOUT) {
        return PYGI_DIRECTION_BIDIRECTIONAL;
    } else if (gi_direction == GI_DIRECTION_IN) {
        if (callable_cache->calling_context != PYGI_CALLING_CONTEXT_IS_FROM_PY)
            return PYGI_DIRECTION_TO_PYTHON;
        return PYGI_DIRECTION_FROM_PYTHON;
    } else {
        if (callable_cache->calling_context != PYGI_CALLING_CONTEXT_IS_FROM_PY)
            return PYGI_DIRECTION_FROM_PYTHON;
        return PYGI_DIRECTION_TO_PYTHON;
    }
}

/* Generate the cache for the callable's arguments */
static gboolean
_callable_cache_generate_args_cache_real (PyGICallableCache *callable_cache,
                                          GICallableInfo *callable_info)
{
    gssize i;
    gssize arg_index;
    GITypeInfo *return_info;
    GITransfer return_transfer;
    PyGIArgCache *return_cache;
    PyGIDirection return_direction;
	gssize last_explicit_arg_index;

    /* Return arguments are always considered out */
    return_direction = _pygi_get_direction (callable_cache, GI_DIRECTION_OUT);

    /* cache the return arg */
    return_info =
        g_callable_info_get_return_type (callable_info);
    return_transfer =
        g_callable_info_get_caller_owns (callable_info);
    return_cache =
        pygi_arg_cache_new (return_info,
                            NULL,
                            return_transfer,
                            return_direction,
                            callable_cache,
                            -1,
                            -1);
    if (return_cache == NULL)
        return FALSE;

    return_cache->is_skipped = g_callable_info_skip_return (callable_info);
    callable_cache->return_cache = return_cache;
    g_base_info_unref (return_info);

    for (i = 0, arg_index = callable_cache->args_offset;
         arg_index < _pygi_callable_cache_args_len (callable_cache);
         i++, arg_index++) {
        PyGIArgCache *arg_cache = NULL;
        GIArgInfo *arg_info;
        PyGIDirection direction;

        arg_info = g_callable_info_get_arg (callable_info, i);

        if (g_arg_info_get_closure (arg_info) == i) {

            arg_cache = pygi_arg_cache_alloc ();
            _pygi_callable_cache_set_arg (callable_cache, arg_index, arg_cache);

            direction = _pygi_get_direction (callable_cache, GI_DIRECTION_IN);
            arg_cache->direction = direction;
            arg_cache->meta_type = PYGI_META_ARG_TYPE_CLOSURE;
            arg_cache->c_arg_index = i;
            arg_cache->is_pointer = TRUE;

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
                /* ensure c_arg_index always aligns with callable_cache->args_cache
                 * and all of the various PyGIInvokeState arrays. */
                arg_cache->c_arg_index = arg_index;

                if (arg_cache->meta_type == PYGI_META_ARG_TYPE_CHILD_WITH_PYARG) {
                    arg_cache->py_arg_index = callable_cache->n_py_args;
                    callable_cache->n_py_args++;
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
                    callable_cache->n_py_args++;
                }

                arg_cache =
                    pygi_arg_cache_new (type_info,
                                        arg_info,
                                        transfer,
                                        direction,
                                        callable_cache,
                                        arg_index,
                                        py_arg_index);

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

    last_explicit_arg_index = -1;

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

static void
_callable_cache_deinit_real (PyGICallableCache *cache)
{
    g_slist_free (cache->to_py_args);
    g_slist_free (cache->arg_name_list);
    g_hash_table_destroy (cache->arg_name_hash);
    g_ptr_array_unref (cache->args_cache);

    if (cache->return_cache != NULL)
        pygi_arg_cache_free (cache->return_cache);
}

static gboolean
_callable_cache_init (PyGICallableCache *cache,
                      GICallableInfo *callable_info)
{
    gint n_args;

    if (cache->deinit == NULL)
        cache->deinit = _callable_cache_deinit_real;

    if (cache->generate_args_cache == NULL)
        cache->generate_args_cache = _callable_cache_generate_args_cache_real;

    cache->name = g_base_info_get_name ((GIBaseInfo *) callable_info);
    cache->throws = g_callable_info_can_throw_gerror ((GIBaseInfo *) callable_info);

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
        PyErr_WarnEx (PyExc_DeprecationWarning, warning, 0);
        g_free (warning);
    }

    n_args = cache->args_offset + g_callable_info_get_n_args (callable_info);

    if (n_args >= 0) {
        cache->args_cache = g_ptr_array_new_full (n_args, (GDestroyNotify) pygi_arg_cache_free);
        g_ptr_array_set_size (cache->args_cache, n_args);
    }

    if (!cache->generate_args_cache (cache, callable_info)) {
        _callable_cache_deinit_real (cache);
        return FALSE;
    }

    return TRUE;
}

void
pygi_callable_cache_free (PyGICallableCache *cache)
{
    cache->deinit (cache);
    g_free (cache);
}

/* PyGIFunctionCache */

static PyObject *
_function_cache_invoke_real (PyGIFunctionCache *function_cache,
                             PyGIInvokeState *state,
                             PyObject *py_args,
                             PyObject *py_kwargs)
{
    return pygi_invoke_c_callable (function_cache, state,
                                   py_args, py_kwargs);
}

static void
_function_cache_deinit_real (PyGICallableCache *callable_cache)
{
    g_function_invoker_destroy (&((PyGIFunctionCache *) callable_cache)->invoker);

    _callable_cache_deinit_real (callable_cache);
}

static gboolean
_function_cache_init (PyGIFunctionCache *function_cache,
                      GICallableInfo *callable_info)
{
    PyGICallableCache *callable_cache = (PyGICallableCache *) function_cache;
    GIFunctionInvoker *invoker = &function_cache->invoker;
    GError *error = NULL;

    callable_cache->calling_context = PYGI_CALLING_CONTEXT_IS_FROM_PY;

    if (callable_cache->deinit == NULL)
        callable_cache->deinit = _function_cache_deinit_real;

    if (function_cache->invoke == NULL)
        function_cache->invoke = _function_cache_invoke_real;

    if (!_callable_cache_init (callable_cache, callable_info))
        return FALSE;

    /* Set by PyGICCallbackCache and PyGIVFuncCache */
    if (invoker->native_address == NULL) {
        if (g_function_info_prep_invoker ((GIFunctionInfo *) callable_info,
                                          invoker,
                                          &error)) {
            return TRUE;
        }
    } else {
        if (g_function_invoker_new_for_address (invoker->native_address,
                                                (GIFunctionInfo *) callable_info,
                                                invoker,
                                                &error)) {
            return TRUE;
        }
    }

    if (!pygi_error_check (&error)) {
        PyErr_Format (PyExc_RuntimeError,
                      "unknown error creating invoker for %s",
                      g_base_info_get_name ((GIBaseInfo *) callable_info));
    }

    _callable_cache_deinit_real (callable_cache);
    return FALSE;
}

PyGIFunctionCache *
pygi_function_cache_new (GICallableInfo *info)
{
    PyGIFunctionCache *function_cache;

    function_cache = g_new0 (PyGIFunctionCache, 1);

    if (!_function_cache_init (function_cache, info)) {
        g_free (function_cache);
        return NULL;
    }

    return function_cache;
}

PyObject *
pygi_function_cache_invoke (PyGIFunctionCache *function_cache,
                            PyObject *py_args,
                            PyObject *py_kwargs)
{
    PyGIInvokeState state = { 0, };

    return function_cache->invoke (function_cache, &state,
                                   py_args, py_kwargs);
}

/* PyGICCallbackCache */

PyGIFunctionCache *
pygi_ccallback_cache_new (GICallableInfo *info,
                          GCallback function_ptr)
{
    PyGICCallbackCache *ccallback_cache;
    PyGIFunctionCache *function_cache;

    ccallback_cache = g_new0 (PyGICCallbackCache, 1);
    function_cache = (PyGIFunctionCache *) ccallback_cache;

    function_cache->invoker.native_address = function_ptr;

    if (!_function_cache_init (function_cache, info)) {
        g_free (ccallback_cache);
        return NULL;
     }

    return function_cache;
}

PyObject *
pygi_ccallback_cache_invoke (PyGICCallbackCache *ccallback_cache,
                             PyObject *py_args,
                             PyObject *py_kwargs,
                             gpointer user_data)
{
    PyGIFunctionCache *function_cache = (PyGIFunctionCache *) ccallback_cache;
    PyGIInvokeState state = { 0, };

    state.user_data = user_data;

    return function_cache->invoke (function_cache, &state,
                                   py_args, py_kwargs);
}

/* PyGIConstructorCache */

static PyObject *
_constructor_cache_invoke_real (PyGIFunctionCache *function_cache,
                                PyGIInvokeState *state,
                                PyObject *py_args,
                                PyObject *py_kwargs)
{
    PyGICallableCache *cache = (PyGICallableCache *) function_cache;
    PyObject *constructor_class;
    PyObject *ret;

    constructor_class = PyTuple_GetItem (py_args, 0);
    if (constructor_class == NULL) {
        PyErr_Clear ();
        PyErr_Format (PyExc_TypeError,
                      "Constructors require the class to be passed in as an argument, "
                      "No arguments passed to the %s constructor.",
                      ((PyGICallableCache *) function_cache)->name);

        return FALSE;
    }

    py_args = PyTuple_GetSlice (py_args, 1, PyTuple_Size (py_args));
    ret = _function_cache_invoke_real (function_cache, state,
                                       py_args, py_kwargs);
    Py_DECREF (py_args);

    if (ret == NULL || cache->return_cache->is_skipped)
        return ret;

    if (ret != Py_None) {
        if (!PyTuple_Check (ret))
            return ret;

        if (PyTuple_GET_ITEM (ret, 0) != Py_None)
            return ret;
    }

    PyErr_SetString (PyExc_TypeError, "constructor returned NULL");

    Py_DECREF (ret);
    return NULL;
}

PyGIFunctionCache *
pygi_constructor_cache_new (GICallableInfo *info)
{
    PyGIConstructorCache *constructor_cache;
    PyGIFunctionCache *function_cache;

    constructor_cache = g_new0 (PyGIConstructorCache, 1);
    function_cache = (PyGIFunctionCache *) constructor_cache;

    function_cache->invoke = _constructor_cache_invoke_real;

    if (!_function_cache_init (function_cache, info)) {
        g_free (constructor_cache);
        return NULL;
    }

    return function_cache;
}

/* PyGIFunctionWithInstanceCache */

static gboolean
_function_with_instance_cache_generate_args_cache_real (PyGICallableCache *callable_cache,
                                                        GICallableInfo *callable_info)
{
    GIInterfaceInfo *interface_info;
    PyGIArgCache *instance_cache;

    interface_info = g_base_info_get_container ((GIBaseInfo *) callable_info);

    instance_cache =
        _arg_cache_new_for_interface (interface_info,
                                      NULL,
                                      NULL,
                                      GI_TRANSFER_NOTHING,
                                      PYGI_DIRECTION_FROM_PYTHON,
                                      callable_cache);

    if (instance_cache == NULL)
        return FALSE;

    /* Because we are not supplied a GITypeInfo for instance arguments,
     * assume some defaults. */
    instance_cache->is_pointer = TRUE;
    instance_cache->py_arg_index = 0;
    instance_cache->c_arg_index = 0;

    _pygi_callable_cache_set_arg (callable_cache, 0, instance_cache);

    callable_cache->n_py_args++;

    return _callable_cache_generate_args_cache_real (callable_cache,
                                                     callable_info);
}

static gboolean
_function_with_instance_cache_init (PyGIFunctionWithInstanceCache *fwi_cache,
                                    GICallableInfo *info)
{
    PyGICallableCache *callable_cache = (PyGICallableCache *) fwi_cache;

    callable_cache->args_offset += 1;
    callable_cache->generate_args_cache = _function_with_instance_cache_generate_args_cache_real;

    return _function_cache_init ((PyGIFunctionCache *) fwi_cache, info);
}

/* PyGIMethodCache */

PyGIFunctionCache *
pygi_method_cache_new (GICallableInfo *info)
{
    PyGIMethodCache *method_cache;
    PyGIFunctionWithInstanceCache *fwi_cache;

    method_cache = g_new0 (PyGIMethodCache, 1);
    fwi_cache = (PyGIFunctionWithInstanceCache *) method_cache;

    if (!_function_with_instance_cache_init (fwi_cache, info)) {
        g_free (method_cache);
        return NULL;
    }

    return (PyGIFunctionCache *) method_cache;
}

/* PyGIVFuncCache */

static PyObject *
_vfunc_cache_invoke_real (PyGIFunctionCache *function_cache,
                          PyGIInvokeState *state,
                          PyObject *py_args,
                          PyObject *py_kwargs)
{
    PyGIVFuncCache *vfunc_cache = (PyGIVFuncCache *) function_cache;
    PyObject *py_gtype;
    GType implementor_gtype;
    GError *error = NULL;
    PyObject *ret;

    py_gtype = PyTuple_GetItem (py_args, 0);
    if (py_gtype == NULL) {
        PyErr_SetString (PyExc_TypeError,
                         "need the GType of the implementor class");
        return FALSE;
    }

    implementor_gtype = pyg_type_from_object (py_gtype);
    if (implementor_gtype == G_TYPE_INVALID)
        return FALSE;

    /* vfunc addresses are pulled into the state at call time and cannot be
     * cached because the call site can specify a different portion of the
     * class hierarchy. e.g. Object.do_func vs. SubObject.do_func might
     * retrieve a different vfunc address but GI gives us the same vfunc info.
     */
    state->function_ptr = g_vfunc_info_get_address ((GIVFuncInfo *) vfunc_cache->info,
                                                    implementor_gtype,
                                                    &error);
    if (pygi_error_check (&error)) {
        return FALSE;
    }

    py_args = PyTuple_GetSlice (py_args, 1, PyTuple_Size (py_args));
    ret = _function_cache_invoke_real (function_cache, state,
                                       py_args, py_kwargs);
    Py_DECREF (py_args);

    return ret;
}

static void
_vfunc_cache_deinit_real (PyGICallableCache *callable_cache)
{
    g_base_info_unref (((PyGIVFuncCache *) callable_cache)->info);

    _function_cache_deinit_real (callable_cache);
}

PyGIFunctionCache *
pygi_vfunc_cache_new (GICallableInfo *info)
{
    PyGIVFuncCache *vfunc_cache;
    PyGIFunctionCache *function_cache;
    PyGIFunctionWithInstanceCache *fwi_cache;

    vfunc_cache = g_new0 (PyGIVFuncCache, 1);
    function_cache = (PyGIFunctionCache *) vfunc_cache;
    fwi_cache = (PyGIFunctionWithInstanceCache *) vfunc_cache;

    ((PyGICallableCache *) vfunc_cache)->deinit = _vfunc_cache_deinit_real;

    /* This must be non-NULL for _function_cache_init() to create the
     * invoker, the real address will be set in _vfunc_cache_invoke_real().
     */
    function_cache->invoker.native_address = (gpointer) 0xdeadbeef;

    function_cache->invoke = _vfunc_cache_invoke_real;

    if (!_function_with_instance_cache_init (fwi_cache, info)) {
        g_free (vfunc_cache);
        return NULL;
    }

    /* Required by _vfunc_cache_invoke_real() */
    vfunc_cache->info = g_base_info_ref ((GIBaseInfo *) info);

    return function_cache;
}

/* PyGIClosureCache */

PyGIClosureCache *
pygi_closure_cache_new (GICallableInfo *info)
{
    gssize i;
    PyGIClosureCache *closure_cache;
    PyGICallableCache *callable_cache;

    closure_cache = g_new0 (PyGIClosureCache, 1);
    callable_cache = (PyGICallableCache *) closure_cache;

    callable_cache->calling_context = PYGI_CALLING_CONTEXT_IS_FROM_C;

    if (!_callable_cache_init (callable_cache, info)) {
        g_free (closure_cache);
        return NULL;
    }

    /* For backwards compatibility closures include the array's length.
     *
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=652115
     */
    for (i = 0; i < _pygi_callable_cache_args_len (callable_cache); i++) {
        PyGIArgCache *arg_cache;
        PyGIArgGArray *garray_cache;
        PyGIArgCache *len_arg_cache;

        arg_cache = g_ptr_array_index (callable_cache->args_cache, i);
        if (arg_cache->type_tag != GI_TYPE_TAG_ARRAY)
            continue;

        garray_cache = (PyGIArgGArray *) arg_cache;
        if (garray_cache->len_arg_index == -1)
            continue;

        len_arg_cache = g_ptr_array_index (callable_cache->args_cache,
                                           garray_cache->len_arg_index);
        len_arg_cache->meta_type = PYGI_META_ARG_TYPE_PARENT;
    }

    return closure_cache;
}

