/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>,  Red Hat, Inc.
 *
 *   pygi-marshal-from-py.c: functions for converting C types to PyObject
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
#include "pygobject-private.h"
#include "pygparamspec.h"

#include <string.h>
#include <time.h>

#include <pyglib.h>
#include <pyglib-python-compat.h>

#include "pygi-cache.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-marshal-to-py.h"
#include "pygi-argument.h"

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

PyObject *
_pygi_marshal_to_py_interface_enum (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
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

PyObject *
_pygi_marshal_to_py_interface_flags (PyGIInvokeState   *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg)
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

PyObject *
_pygi_marshal_to_py_interface_struct_cache_adapter (PyGIInvokeState   *state,
                                                    PyGICallableCache *callable_cache,
                                                    PyGIArgCache      *arg_cache,
                                                    GIArgument        *arg)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    return _pygi_marshal_to_py_interface_struct (arg,
                                                 iface_cache->interface_info,
                                                 iface_cache->g_type,
                                                 iface_cache->py_type,
                                                 arg_cache->transfer,
                                                 arg_cache->is_caller_allocates,
                                                 iface_cache->is_foreign);
}

PyObject *
_pygi_marshal_to_py_interface_interface (PyGIInvokeState   *state,
                                         PyGICallableCache *callable_cache,
                                         PyGIArgCache      *arg_cache,
                                         GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_boxed (PyGIInvokeState   *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_union  (PyGIInvokeState   *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache      *arg_cache,
                                      GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_struct (GIArgument *arg,
                                      GIInterfaceInfo *interface_info,
                                      GType g_type,
                                      PyObject *py_type,
                                      GITransfer transfer,
                                      gboolean is_allocated,
                                      gboolean is_foreign)
{
    PyObject *py_obj = NULL;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        py_obj = pyg_value_as_pyobject (arg->v_pointer, FALSE);
    } else if (is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (interface_info,
                                                              arg->v_pointer);
    } else if (g_type_is_a (g_type, G_TYPE_BOXED)) {
        if (py_type) {
            py_obj = _pygi_boxed_new ((PyTypeObject *) py_type,
                                      arg->v_pointer,
                                      transfer == GI_TRANSFER_EVERYTHING || is_allocated,
                                      is_allocated ?
                                              g_struct_info_get_size(interface_info) : 0);
        }
    } else if (g_type_is_a (g_type, G_TYPE_POINTER)) {
        if (py_type == NULL ||
                !PyType_IsSubtype ((PyTypeObject *) py_type, &PyGIStruct_Type)) {
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            py_obj = pyg_pointer_new (g_type, arg->v_pointer);
        } else {
            py_obj = _pygi_struct_new ( (PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       transfer == GI_TRANSFER_EVERYTHING);
        }
    } else if (g_type_is_a (g_type, G_TYPE_VARIANT)) {
        /* Note: sink the variant (add a ref) only if we are not transfered ownership.
         * GLib.Variant overrides __del__ which will then call "g_variant_unref" for
         * cleanup in either case. */
        if (py_type) {
            if (transfer == GI_TRANSFER_NOTHING) {
                g_variant_ref_sink (arg->v_pointer);
            }
            py_obj = _pygi_struct_new ((PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       FALSE);
        }
    } else if (g_type == G_TYPE_NONE) {
        if (py_type) {
            py_obj = _pygi_struct_new ((PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       transfer == GI_TRANSFER_EVERYTHING);
        }
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (g_type));
    }

    return py_obj;
}
