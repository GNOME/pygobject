/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
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

#include "pygi-argument.h"
#include "pygi-basictype.h"
#include "pygi-cache-private.h"
#include "pygi-type.h"


/* pygi_arg_base_setup:
 * arg_cache: argument cache to initialize
 * type_info: source for type related attributes to cache
 * arg_info: (allow-none): source for argument related attributes to cache
 * transfer: transfer mode to store in the argument cache
 * direction: marshaling direction to store in the cache
 *
 * Initializer for PyGIArgCache
 */
void
pygi_arg_base_setup (
    PyGIArgCache *arg_cache, GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction)
{
    arg_cache->direction = direction;
    arg_cache->transfer = transfer;
    arg_cache->py_arg_index = -1;
    arg_cache->c_arg_index = -1;

    if (type_info != NULL) {
        arg_cache->is_pointer = gi_type_info_is_pointer (type_info);
        arg_cache->type_tag = gi_type_info_get_tag (type_info);
        gi_base_info_ref ((GIBaseInfo *)type_info);
        arg_cache->type_info = type_info;
    }

    if (arg_info != NULL) {
        gi_base_info_ref ((GIBaseInfo *)arg_info);
        arg_cache->arg_info = arg_info;
    }
}

static GIArgument
pygi_marshal_from_py_basic_type (PyObject *object, /* in */
                                 GITypeTag type_tag, GITransfer transfer,
                                 gpointer *cleanup_data /* out */)
{
    GIArgument arg = PYGI_ARG_INIT;
    gboolean converted = FALSE;

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
        if (pygi_gpointer_from_py (object, &(arg.v_pointer))) {
            *cleanup_data = arg.v_pointer;
            converted = TRUE;
        }
        break;

    case GI_TYPE_TAG_INT8:
        converted = pygi_gint8_from_py (object, &(arg.v_int8));
        break;
    case GI_TYPE_TAG_UINT8:
        converted = pygi_guint8_from_py (object, &(arg.v_uint8));
        break;
    case GI_TYPE_TAG_INT16:
        converted = pygi_gint16_from_py (object, &(arg.v_int16));
        break;
    case GI_TYPE_TAG_UINT16:
        converted = pygi_guint16_from_py (object, &(arg.v_uint16));
        break;
    case GI_TYPE_TAG_INT32:
        converted = pygi_gint32_from_py (object, &(arg.v_int32));
        break;
    case GI_TYPE_TAG_UINT32:
        converted = pygi_guint32_from_py (object, &(arg.v_uint32));
        break;
    case GI_TYPE_TAG_INT64:
        converted = pygi_gint64_from_py (object, &(arg.v_int64));
        break;
    case GI_TYPE_TAG_UINT64:
        converted = pygi_guint64_from_py (object, &(arg.v_uint64));
        break;
    case GI_TYPE_TAG_BOOLEAN:
        converted = pygi_gboolean_from_py (object, &(arg.v_boolean));
        break;
    case GI_TYPE_TAG_FLOAT:
        converted = pygi_gfloat_from_py (object, &(arg.v_float));
        break;
    case GI_TYPE_TAG_DOUBLE:
        converted = pygi_gdouble_from_py (object, &(arg.v_double));
        break;
    case GI_TYPE_TAG_GTYPE:
        converted = pygi_gtype_from_py (object, &(arg.v_size));
        break;
    case GI_TYPE_TAG_UNICHAR:
        converted = pygi_gunichar_from_py (object, &(arg.v_uint32));
        break;
    case GI_TYPE_TAG_UTF8:
        if (pygi_utf8_from_py (object, &(arg.v_string))) {
            *cleanup_data = arg.v_string;
            converted = TRUE;
        }
        break;

    case GI_TYPE_TAG_FILENAME:
        if (pygi_filename_from_py (object, &(arg.v_string))) {
            *cleanup_data = arg.v_string;
            converted = TRUE;
        }
        break;

    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        PyErr_Format (PyExc_TypeError, "Type tag %s (%d) not supported",
                      gi_type_tag_to_string (type_tag), type_tag);
        break;
    default:
        g_assert_not_reached ();
    }

    if (!converted && !PyErr_Occurred ()) {
        PyErr_Format (PyExc_TypeError,
                      "Object %R Could not be converted type tag %s", object,
                      gi_type_tag_to_string (type_tag));
    }

    return arg;
}

/**
 * pygi_marshal_to_py_basic_type:
 * @arg: The argument to convert to an object.
 * @type_tag: Type tag for @arg
 * @transfer: Transfer annotation
 *
 * Convert the given argument to a Python object. This function
 * is restricted to simple types that only require the GITypeTag
 * and GITransfer. For a more complete conversion routine, use:
 * pygi_argument_to_py.
 *
 * Returns: A PyObject representing @arg or NULL if it cannot convert
 *          the argument.
 */
static PyObject *
pygi_marshal_to_py_basic_type (GIArgument arg, GITypeTag type_tag,
                               GITransfer transfer)
{
    switch (type_tag) {
    case GI_TYPE_TAG_BOOLEAN:
        return pygi_gboolean_to_py (arg.v_boolean);

    case GI_TYPE_TAG_INT8:
        return pygi_gint8_to_py (arg.v_int8);

    case GI_TYPE_TAG_UINT8:
        return pygi_guint8_to_py (arg.v_uint8);

    case GI_TYPE_TAG_INT16:
        return pygi_gint16_to_py (arg.v_int16);

    case GI_TYPE_TAG_UINT16:
        return pygi_guint16_to_py (arg.v_uint16);

    case GI_TYPE_TAG_INT32:
        return pygi_gint32_to_py (arg.v_int32);

    case GI_TYPE_TAG_UINT32:
        return pygi_guint32_to_py (arg.v_uint32);

    case GI_TYPE_TAG_INT64:
        return pygi_gint64_to_py (arg.v_int64);

    case GI_TYPE_TAG_UINT64:
        return pygi_guint64_to_py (arg.v_uint64);

    case GI_TYPE_TAG_FLOAT:
        return pygi_gfloat_to_py (arg.v_float);

    case GI_TYPE_TAG_DOUBLE:
        return pygi_gdouble_to_py (arg.v_double);

    case GI_TYPE_TAG_GTYPE:
        return pyg_type_wrapper_new ((GType)arg.v_size);

    case GI_TYPE_TAG_UNICHAR:
        return pygi_gunichar_to_py (arg.v_uint32);

    case GI_TYPE_TAG_UTF8:
        return pygi_utf8_to_py (arg.v_string);

    case GI_TYPE_TAG_FILENAME:
        return pygi_filename_to_py (arg.v_string);

    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        PyErr_Format (PyExc_TypeError, "Type tag %d not supported", type_tag);
        return NULL;
    default:
        g_assert_not_reached ();
    }
}

gboolean
pygi_marshal_from_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, PyObject *py_arg, GIArgument *arg,
    PyGIMarshalCleanupData *cleanup_data)
{
    *arg = pygi_marshal_from_py_basic_type (py_arg, arg_cache->type_tag,
                                            arg_cache->transfer,
                                            &(cleanup_data->data));
    return !PyErr_Occurred ();
}

PyObject *
pygi_marshal_to_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg,
    PyGIMarshalCleanupData *cleanup_data)
{
    return pygi_marshal_to_py_basic_type (*arg, arg_cache->type_tag,
                                          arg_cache->transfer);
}

static gboolean
marshal_from_py_void (PyGIInvokeState *state,
                      PyGICallableCache *callable_cache,
                      PyGIArgCache *arg_cache, PyObject *py_arg,
                      GIArgument *arg, PyGIMarshalCleanupData *cleanup_data)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    return pygi_gpointer_from_py (py_arg, &(arg->v_pointer));
}

static PyObject *
marshal_to_py_void (PyGIInvokeState *state, PyGICallableCache *callable_cache,
                    PyGIArgCache *arg_cache, GIArgument *arg,
                    PyGIMarshalCleanupData *cleanup_data)
{
    if (arg_cache->is_pointer) {
        return PyLong_FromVoidPtr (arg->v_pointer);
    }
    Py_RETURN_NONE;
}


static gboolean
pygi_marshal_from_py_utf8_cache_adapter (PyGIInvokeState *state,
                                         PyGICallableCache *callable_cache,
                                         PyGIArgCache *arg_cache,
                                         PyObject *py_arg, GIArgument *arg,
                                         PyGIMarshalCleanupData *cleanup_data)
{
    *arg = pygi_marshal_from_py_basic_type (py_arg, arg_cache->type_tag,
                                            arg_cache->transfer,
                                            &(cleanup_data->data));

    /* We strdup strings so free unless ownership is transferred to C. */
    if (cleanup_data->data != NULL)
        pygi_marshal_cleanup_data_init_full (
            cleanup_data, arg->v_pointer,
            arg_cache->transfer == GI_TRANSFER_NOTHING ? g_free : NULL,
            g_free);

    return !PyErr_Occurred ();
}

static PyObject *
pygi_marshal_to_py_utf8_cache_adapter (PyGIInvokeState *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache *arg_cache,
                                       GIArgument *arg,
                                       PyGIMarshalCleanupData *cleanup_data)
{
    PyObject *object = pygi_marshal_to_py_basic_type (
        *arg, arg_cache->type_tag, arg_cache->transfer);

    /* Python copies the string so we need to free it
     * if the interface is transfering ownership,
     * whether or not it has been processed yet */
    pygi_marshal_cleanup_data_init_full (
        cleanup_data, arg->v_pointer,
        arg_cache->transfer == GI_TRANSFER_EVERYTHING ? g_free : NULL,
        arg_cache->transfer == GI_TRANSFER_EVERYTHING ? g_free : NULL);

    return object;
}

PyGIArgCache *
pygi_arg_void_type_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                                  GITransfer transfer, PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = marshal_from_py_void;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = marshal_to_py_void;

    return arg_cache;
}

PyGIArgCache *
pygi_arg_numeric_type_new_from_info (GITypeInfo *type_info,
                                     GIArgInfo *arg_info, GITransfer transfer,
                                     PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller =
            pygi_marshal_from_py_basic_type_cache_adapter;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller =
            pygi_marshal_to_py_basic_type_cache_adapter;

    return arg_cache;
}


PyGIArgCache *
pygi_arg_string_type_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                                    GITransfer transfer,
                                    PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller =
            pygi_marshal_from_py_utf8_cache_adapter;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = pygi_marshal_to_py_utf8_cache_adapter;
    }

    return arg_cache;
}
