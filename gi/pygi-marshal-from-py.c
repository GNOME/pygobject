/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>,  Red Hat, Inc.
 *
 *   pygi-marshal-from-py.c: Functions to convert PyObjects to C types.
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

#include "pygi-private.h"

#include <string.h>
#include <time.h>
#include <pygobject.h>
#include <pyglib-python-compat.h>

#include "pygi-cache.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-marshal-from-py.h"

#ifdef _WIN32
#ifdef _MSC_VER
#include <math.h>

#ifndef NAN
static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
#define NAN (*(const float *) __nan)
#endif

#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif

#endif
#endif

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

gboolean
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

gboolean
_pygi_marshal_from_py_interface_flags (PyGIInvokeState   *state,
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
    GIBaseInfo *interface;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (py_long == NULL) {
        PyErr_Clear ();
        goto err;
    }

    c_long = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    /* only 0 or argument of type Flag is allowed */
    if (!is_instance && c_long != 0)
        goto err;

    /* Write c_long into arg */
    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_FLAGS);
    if (!gi_argument_from_c_long(arg, c_long,
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

