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

#include <Python.h>
#include <glib.h>
#include <pyglib-python-compat.h>

#include "pygi-enum-marshal.h"
#include "pygi-type.h"
#include "pygenum.h"
#include "pygflags.h"

static gboolean
gi_argument_from_c_long (GIArgument *arg_out,
                         long        c_long_in,
                         GITypeTag   type_tag)
{
    switch (type_tag) {
      case GI_TYPE_TAG_INT8:
          arg_out->v_int8 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT8:
          arg_out->v_uint8 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_INT16:
          arg_out->v_int16 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT16:
          arg_out->v_uint16 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_INT32:
          arg_out->v_int32 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT32:
          arg_out->v_uint32 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_INT64:
          arg_out->v_int64 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT64:
          arg_out->v_uint64 = c_long_in;
          return TRUE;
      default:
          PyErr_Format (PyExc_TypeError,
                        "Unable to marshal C long %ld to %s",
                        c_long_in,
                        g_type_tag_to_string (type_tag));
          return FALSE;
    }
}

static gboolean
gi_argument_to_c_long (GIArgument *arg_in,
                       long *c_long_out,
                       GITypeTag type_tag)
{
    switch (type_tag) {
      case GI_TYPE_TAG_INT8:
          *c_long_out = arg_in->v_int8;
          return TRUE;
      case GI_TYPE_TAG_UINT8:
          *c_long_out = arg_in->v_uint8;
          return TRUE;
      case GI_TYPE_TAG_INT16:
          *c_long_out = arg_in->v_int16;
          return TRUE;
      case GI_TYPE_TAG_UINT16:
          *c_long_out = arg_in->v_uint16;
          return TRUE;
      case GI_TYPE_TAG_INT32:
          *c_long_out = arg_in->v_int32;
          return TRUE;
      case GI_TYPE_TAG_UINT32:
          *c_long_out = arg_in->v_uint32;
          return TRUE;
      case GI_TYPE_TAG_INT64:
          *c_long_out = arg_in->v_int64;
          return TRUE;
      case GI_TYPE_TAG_UINT64:
          *c_long_out = arg_in->v_uint64;
          return TRUE;
      default:
          PyErr_Format (PyExc_TypeError,
                        "Unable to marshal %s to C long",
                        g_type_tag_to_string (type_tag));
          return FALSE;
    }
}

static gboolean
_pygi_marshal_from_py_interface_enum (PyGIInvokeState   *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache      *arg_cache,
                                      PyObject          *py_arg,
                                      GIArgument        *arg,
                                      gpointer          *cleanup_data)
{
    PyObject *py_long;
    long c_long;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface = NULL;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (py_long == NULL) {
        PyErr_Clear();
        goto err;
    }

    c_long = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    /* Write c_long into arg */
    interface = g_type_info_get_interface (arg_cache->type_info);
    assert(g_base_info_get_type (interface) == GI_INFO_TYPE_ENUM);
    if (!gi_argument_from_c_long(arg,
                                 c_long,
                                 g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
          g_assert_not_reached();
          g_base_info_unref (interface);
          return FALSE;
    }

    /* If this is not an instance of the Enum type that we want
     * we need to check if the value is equivilant to one of the
     * Enum's memebers */
    if (!is_instance) {
        int i;
        gboolean is_found = FALSE;

        for (i = 0; i < g_enum_info_get_n_values (iface_cache->interface_info); i++) {
            GIValueInfo *value_info =
                g_enum_info_get_value (iface_cache->interface_info, i);
            glong enum_value = g_value_info_get_value (value_info);
            g_base_info_unref ( (GIBaseInfo *)value_info);
            if (c_long == enum_value) {
                is_found = TRUE;
                break;
            }
        }

        if (!is_found)
            goto err;
    }

    g_base_info_unref (interface);
    return TRUE;

err:
    if (interface)
        g_base_info_unref (interface);
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;
}

static gboolean
_pygi_marshal_from_py_interface_flags (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg,
                                       gpointer          *cleanup_data)
{
    PyObject *py_long;
    unsigned long c_ulong;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (py_long == NULL) {
        PyErr_Clear ();
        goto err;
    }

    c_ulong = PYGLIB_PyLong_AsUnsignedLong (py_long);
    Py_DECREF (py_long);

    /* only 0 or argument of type Flag is allowed */
    if (!is_instance && c_ulong != 0)
        goto err;

    /* Write c_long into arg */
    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_FLAGS);
    if (!gi_argument_from_c_long(arg, c_ulong,
                                 g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        g_base_info_unref (interface);
        return FALSE;
    }

    g_base_info_unref (interface);
    return TRUE;

err:
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;

}

static PyObject *
_pygi_marshal_to_py_interface_enum (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg,
                                    gpointer          *cleanup_data)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;
    long c_long;

    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_ENUM);

    if (!gi_argument_to_c_long(arg, &c_long,
                               g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        return NULL;
    }

    if (iface_cache->g_type == G_TYPE_NONE) {
        py_obj = PyObject_CallFunction (iface_cache->py_type, "l", c_long);
    } else {
        py_obj = pyg_enum_from_gtype (iface_cache->g_type, c_long);
    }
    g_base_info_unref (interface);
    return py_obj;
}

static PyObject *
_pygi_marshal_to_py_interface_flags (PyGIInvokeState   *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg,
                                     gpointer          *cleanup_data)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;
    long c_long;

    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_FLAGS);

    if (!gi_argument_to_c_long(arg, &c_long,
                               g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        g_base_info_unref (interface);
        return NULL;
    }

    g_base_info_unref (interface);
    if (iface_cache->g_type == G_TYPE_NONE) {
        /* An enum with a GType of None is an enum without GType */

        PyObject *py_type = _pygi_type_import_by_gi_info (iface_cache->interface_info);
        PyObject *py_args = NULL;

        if (!py_type)
            return NULL;

        py_args = PyTuple_New (1);
        if (PyTuple_SetItem (py_args, 0, PyLong_FromLong (c_long)) != 0) {
            Py_DECREF (py_args);
            Py_DECREF (py_type);
            return NULL;
        }

        py_obj = PyObject_CallFunction (py_type, "l", c_long);

        Py_DECREF (py_args);
        Py_DECREF (py_type);
    } else {
        py_obj = pyg_flags_from_gtype (iface_cache->g_type, c_long);
    }

    return py_obj;
}

static gboolean
pygi_arg_enum_setup_from_info (PyGIArgCache  *arg_cache,
                               GITypeInfo    *type_info,
                               GIArgInfo     *arg_info,
                               GITransfer     transfer,
                               PyGIDirection  direction)
{
    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_enum;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_enum;

    return TRUE;
}


PyGIArgCache *
pygi_arg_enum_new_from_info (GITypeInfo      *type_info,
                             GIArgInfo       *arg_info,
                             GITransfer       transfer,
                             PyGIDirection    direction,
                             GIInterfaceInfo *iface_info)
{
    gboolean res = FALSE;
    PyGIArgCache *cache = NULL;

    cache = pygi_arg_interface_new_from_info (type_info,
                                              arg_info,
                                              transfer,
                                              direction,
                                              iface_info);
    if (cache == NULL)
        return NULL;

    res = pygi_arg_enum_setup_from_info (cache,
                                         type_info,
                                         arg_info,
                                         transfer,
                                         direction);
    if (res) {
        return cache;
    } else {
        pygi_arg_cache_free (cache);
        return NULL;
    }
}

static gboolean
pygi_arg_flags_setup_from_info (PyGIArgCache  *arg_cache,
                                GITypeInfo    *type_info,
                                GIArgInfo     *arg_info,
                                GITransfer     transfer,
                                PyGIDirection  direction)
{
    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_flags;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_flags;

    return TRUE;
}


PyGIArgCache *
pygi_arg_flags_new_from_info (GITypeInfo      *type_info,
                              GIArgInfo       *arg_info,
                              GITransfer       transfer,
                              PyGIDirection    direction,
                              GIInterfaceInfo *iface_info)
{
    gboolean res = FALSE;
    PyGIArgCache *cache = NULL;

    cache = pygi_arg_interface_new_from_info (type_info,
                                              arg_info,
                                              transfer,
                                              direction,
                                              iface_info);
    if (cache == NULL)
        return NULL;

    res = pygi_arg_flags_setup_from_info (cache,
                                          type_info,
                                          arg_info,
                                          transfer,
                                          direction);
    if (res) {
        return cache;
    } else {
        pygi_arg_cache_free (cache);
        return NULL;
    }
}
