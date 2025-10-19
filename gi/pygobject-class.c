/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygobject-typeinfo.c: everything needed to set up
 *     Python classes derived from GObject.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <glib-object.h>
#include <pythoncapi_compat.h>
#include <girepository/girepository.h>

#include "pygobject-internal.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pygi-basictype.h"
#include "pygi-fundamental.h"
#include "pygi-property.h"
#include "pygi-type.h"
#include "pygi-util.h"
#include "pygi-value.h"
#include "pygobject-object.h"


typedef struct _PyGSignalAccumulatorData {
    PyObject *callable;
    PyObject *user_data;
} PyGSignalAccumulatorData;


static gboolean
_pyg_signal_accumulator (GSignalInvocationHint *ihint, GValue *return_accu,
                         const GValue *handler_return, gpointer _data)
{
    PyObject *py_ihint, *py_return_accu, *py_handler_return, *py_detail;
    PyObject *py_retval;
    gboolean retval = FALSE;
    PyGSignalAccumulatorData *data = _data;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    if (ihint->detail)
        py_detail = PyUnicode_FromString (g_quark_to_string (ihint->detail));
    else {
        py_detail = Py_NewRef (Py_None);
    }

    py_ihint = Py_BuildValue ("lNi", (long int)ihint->signal_id, py_detail,
                              ihint->run_type);
    py_handler_return = pyg_value_as_pyobject (handler_return, TRUE);
    py_return_accu = pyg_value_as_pyobject (return_accu, FALSE);
    if (data->user_data)
        py_retval = PyObject_CallFunction (data->callable, "NNNO", py_ihint,
                                           py_return_accu, py_handler_return,
                                           data->user_data);
    else
        py_retval = PyObject_CallFunction (data->callable, "NNN", py_ihint,
                                           py_return_accu, py_handler_return);
    if (!py_retval)
        PyErr_Print ();
    else {
        if (!PyTuple_Check (py_retval) || PyTuple_Size (py_retval) != 2) {
            PyErr_SetString (PyExc_TypeError,
                             "accumulator function must return"
                             " a (bool, object) tuple");
            PyErr_Print ();
        } else {
            retval = PyObject_IsTrue (PyTuple_GET_ITEM (py_retval, 0));
            if (pyg_value_from_pyobject (return_accu,
                                         PyTuple_GET_ITEM (py_retval, 1))) {
                PyErr_Print ();
            }
        }
        Py_DECREF (py_retval);
    }
    PyGILState_Release (state);
    return retval;
}

static gboolean
override_signal (GType instance_type, const gchar *signal_name)
{
    guint signal_id;

    signal_id = g_signal_lookup (signal_name, instance_type);
    if (!signal_id) {
        gchar buf[128];

        g_snprintf (buf, sizeof (buf), "could not look up %s", signal_name);
        PyErr_SetString (PyExc_TypeError, buf);
        return FALSE;
    }
    g_signal_override_class_closure (signal_id, instance_type,
                                     pyg_signal_class_closure_get ());
    return TRUE;
}

static gboolean
create_signal (GType instance_type, const gchar *signal_name, PyObject *tuple)
{
    GSignalFlags signal_flags;
    PyObject *py_return_type, *py_param_types;
    GType return_type;
    guint n_params, i;
    Py_ssize_t py_n_params;
    GType *param_types;
    guint signal_id;
    GSignalAccumulator accumulator = NULL;
    PyGSignalAccumulatorData *accum_data = NULL;
    PyObject *py_accum = NULL, *py_accum_data = NULL;

    if (!PyArg_ParseTuple (tuple, "iOO|OO", &signal_flags, &py_return_type,
                           &py_param_types, &py_accum, &py_accum_data)) {
        gchar buf[128];

        PyErr_Clear ();
        g_snprintf (buf, sizeof (buf),
                    "value for __gsignals__['%s'] not in correct format",
                    signal_name);
        PyErr_SetString (PyExc_TypeError, buf);
        return FALSE;
    }

    if (py_accum && !Py_IsNone (py_accum) && !PyCallable_Check (py_accum)) {
        gchar buf[128];

        g_snprintf (buf, sizeof (buf),
                    "accumulator for __gsignals__['%s'] must be callable",
                    signal_name);
        PyErr_SetString (PyExc_TypeError, buf);
        return FALSE;
    }

    return_type = pyg_type_from_object (py_return_type);
    if (!return_type) return FALSE;
    if (!PySequence_Check (py_param_types)) {
        gchar buf[128];

        g_snprintf (
            buf, sizeof (buf),
            "third element of __gsignals__['%s'] tuple must be a sequence",
            signal_name);
        PyErr_SetString (PyExc_TypeError, buf);
        return FALSE;
    }
    py_n_params = PySequence_Length (py_param_types);
    if (py_n_params < 0) return FALSE;

    if (!pygi_guint_from_pyssize (py_n_params, &n_params)) return FALSE;

    param_types = g_new (GType, n_params);
    for (i = 0; i < n_params; i++) {
        PyObject *item = PySequence_GetItem (py_param_types, i);

        param_types[i] = pyg_type_from_object (item);
        if (param_types[i] == 0) {
            Py_DECREF (item);
            g_free (param_types);
            return FALSE;
        }
        Py_DECREF (item);
    }

    if (py_accum != NULL && !Py_IsNone (py_accum)) {
        accum_data = g_new (PyGSignalAccumulatorData, 1);
        accum_data->callable = Py_NewRef (py_accum);
        accum_data->user_data = Py_XNewRef (py_accum_data);
        accumulator = _pyg_signal_accumulator;
    }

    signal_id = g_signal_newv (signal_name, instance_type, signal_flags,
                               pyg_signal_class_closure_get (), accumulator,
                               accum_data, gi_cclosure_marshal_generic,
                               return_type, n_params, param_types);
    g_free (param_types);

    if (signal_id == 0) {
        gchar buf[128];

        g_snprintf (buf, sizeof (buf), "could not create signal for %s",
                    signal_name);
        PyErr_SetString (PyExc_RuntimeError, buf);
        return FALSE;
    }
    return TRUE;
}

static PyObject *
add_signals (GObjectClass *klass, PyObject *signals)
{
    gboolean ret = TRUE;
    Py_ssize_t pos = 0;
    PyObject *key, *value, *overridden_signals = NULL;
    GType instance_type = G_OBJECT_CLASS_TYPE (klass);

    overridden_signals = PyDict_New ();
    while (PyDict_Next (signals, &pos, &key, &value)) {
        const gchar *signal_name;
        gchar *signal_name_canon, *c;

        if (!PyUnicode_Check (key)) {
            PyErr_SetString (PyExc_TypeError,
                             "__gsignals__ keys must be strings");
            ret = FALSE;
            break;
        }
        signal_name = PyUnicode_AsUTF8 (key);

        if (Py_IsNone (value)
            || (PyUnicode_Check (value)
                && !strcmp (PyUnicode_AsUTF8 (value), "override"))) {
            /* canonicalize signal name, replacing '-' with '_' */
            signal_name_canon = g_strdup (signal_name);
            for (c = signal_name_canon; *c; ++c)
                if (*c == '-') *c = '_';
            if (PyDict_SetItemString (overridden_signals, signal_name_canon,
                                      key)) {
                g_free (signal_name_canon);
                ret = FALSE;
                break;
            }
            g_free (signal_name_canon);

            ret = override_signal (instance_type, signal_name);
        } else {
            ret = create_signal (instance_type, signal_name, value);
        }

        if (!ret) break;
    }
    if (ret)
        return overridden_signals;
    else {
        Py_XDECREF (overridden_signals);
        return NULL;
    }
}

static GParamSpec *
create_property (const gchar *prop_name, GType prop_type, const gchar *nick,
                 const gchar *blurb, PyObject *args, GParamFlags flags)
{
    GParamSpec *pspec = NULL;

    switch (G_TYPE_FUNDAMENTAL (prop_type)) {
    case G_TYPE_CHAR: {
        gchar minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "ccc", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_char (prop_name, nick, blurb, minimum, maximum,
                                   default_value, flags);
        break;
    }
    case G_TYPE_UCHAR: {
        gchar minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "ccc", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_uchar (prop_name, nick, blurb, minimum, maximum,
                                    default_value, flags);
        break;
    }
    case G_TYPE_BOOLEAN: {
        gboolean default_value;

        if (!PyArg_ParseTuple (args, "i", &default_value)) return NULL;
        pspec = g_param_spec_boolean (prop_name, nick, blurb, default_value,
                                      flags);
        break;
    }
    case G_TYPE_INT: {
        gint minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "iii", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_int (prop_name, nick, blurb, minimum, maximum,
                                  default_value, flags);
        break;
    }
    case G_TYPE_UINT: {
        guint minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "III", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_uint (prop_name, nick, blurb, minimum, maximum,
                                   default_value, flags);
        break;
    }
    case G_TYPE_LONG: {
        glong minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "lll", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_long (prop_name, nick, blurb, minimum, maximum,
                                   default_value, flags);
        break;
    }
    case G_TYPE_ULONG: {
        gulong minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "kkk", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_ulong (prop_name, nick, blurb, minimum, maximum,
                                    default_value, flags);
        break;
    }
    case G_TYPE_INT64: {
        gint64 minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "LLL", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_int64 (prop_name, nick, blurb, minimum, maximum,
                                    default_value, flags);
        break;
    }
    case G_TYPE_UINT64: {
        guint64 minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "KKK", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_uint64 (prop_name, nick, blurb, minimum, maximum,
                                     default_value, flags);
        break;
    }
    case G_TYPE_ENUM: {
        gint default_value;
        PyObject *pydefault;

        if (!PyArg_ParseTuple (args, "O", &pydefault)) return NULL;

        if (pyg_enum_get_value (prop_type, pydefault, (gint *)&default_value))
            return NULL;

        pspec = g_param_spec_enum (prop_name, nick, blurb, prop_type,
                                   default_value, flags);
        break;
    }
    case G_TYPE_FLAGS: {
        guint default_value;
        PyObject *pydefault;

        if (!PyArg_ParseTuple (args, "O", &pydefault)) return NULL;

        if (pyg_flags_get_value (prop_type, pydefault, &default_value))
            return NULL;

        pspec = g_param_spec_flags (prop_name, nick, blurb, prop_type,
                                    default_value, flags);
        break;
    }
    case G_TYPE_FLOAT: {
        gfloat minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "fff", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_float (prop_name, nick, blurb, minimum, maximum,
                                    default_value, flags);
        break;
    }
    case G_TYPE_DOUBLE: {
        gdouble minimum, maximum, default_value;

        if (!PyArg_ParseTuple (args, "ddd", &minimum, &maximum,
                               &default_value))
            return NULL;
        pspec = g_param_spec_double (prop_name, nick, blurb, minimum, maximum,
                                     default_value, flags);
        break;
    }
    case G_TYPE_STRING: {
        const gchar *default_value;

        if (!PyArg_ParseTuple (args, "z", &default_value)) return NULL;
        pspec =
            g_param_spec_string (prop_name, nick, blurb, default_value, flags);
        break;
    }
    case G_TYPE_PARAM:
        if (!PyArg_ParseTuple (args, "")) return NULL;
        pspec = g_param_spec_param (prop_name, nick, blurb, prop_type, flags);
        break;
    case G_TYPE_BOXED:
        if (!PyArg_ParseTuple (args, "")) return NULL;
        pspec = g_param_spec_boxed (prop_name, nick, blurb, prop_type, flags);
        break;
    case G_TYPE_POINTER:
        if (!PyArg_ParseTuple (args, "")) return NULL;
        if (prop_type == G_TYPE_GTYPE)
            pspec = g_param_spec_gtype (prop_name, nick, blurb, G_TYPE_NONE,
                                        flags);
        else
            pspec = g_param_spec_pointer (prop_name, nick, blurb, flags);
        break;
    case G_TYPE_OBJECT:
    case G_TYPE_INTERFACE:
        if (!PyArg_ParseTuple (args, "")) return NULL;
        pspec = g_param_spec_object (prop_name, nick, blurb, prop_type, flags);
        break;
    case G_TYPE_VARIANT: {
        PyObject *pydefault;
        GVariant *default_value = NULL;

        if (!PyArg_ParseTuple (args, "O", &pydefault)) return NULL;
        if (!Py_IsNone (pydefault))
            default_value = pyg_boxed_get (pydefault, GVariant);
        pspec = g_param_spec_variant (
            prop_name, nick, blurb, G_VARIANT_TYPE_ANY, default_value, flags);
        break;
    }
    default:
        /* unhandled pspec type ... */
        break;
    }

    if (!pspec) {
        char buf[128];

        g_snprintf (buf, sizeof (buf),
                    "could not create param spec for type %s",
                    g_type_name (prop_type));
        PyErr_SetString (PyExc_TypeError, buf);
        return NULL;
    }

    return pspec;
}

static gboolean
add_properties (GObjectClass *klass, PyObject *properties)
{
    gboolean ret = TRUE;
    Py_ssize_t pos = 0;
    PyObject *key, *value;

    while (PyDict_Next (properties, &pos, &key, &value)) {
        const gchar *prop_name;
        GType prop_type;
        const gchar *nick, *blurb;
        GParamFlags flags;
        Py_ssize_t val_length;
        PyObject *slice, *item, *py_prop_type;
        GParamSpec *pspec;

        /* values are of format (type,nick,blurb, type_specific_args, flags) */

        if (!PyUnicode_Check (key)) {
            PyErr_SetString (PyExc_TypeError,
                             "__gproperties__ keys must be strings");
            ret = FALSE;
            break;
        }
        prop_name = PyUnicode_AsUTF8 (key);

        if (!PyTuple_Check (value)) {
            PyErr_SetString (PyExc_TypeError,
                             "__gproperties__ values must be tuples");
            ret = FALSE;
            break;
        }
        val_length = PyTuple_Size (value);
        if (val_length < 4) {
            PyErr_SetString (
                PyExc_TypeError,
                "__gproperties__ values must be at least 4 elements long");
            ret = FALSE;
            break;
        }

        slice = PySequence_GetSlice (value, 0, 3);
        if (!slice) {
            ret = FALSE;
            break;
        }
        if (!PyArg_ParseTuple (slice, "Ozz", &py_prop_type, &nick, &blurb)) {
            Py_DECREF (slice);
            ret = FALSE;
            break;
        }
        Py_DECREF (slice);
        prop_type = pyg_type_from_object (py_prop_type);
        if (!prop_type) {
            ret = FALSE;
            break;
        }
        item = PyTuple_GetItem (value, val_length - 1);
        if (!PyLong_Check (item)) {
            PyErr_SetString (
                PyExc_TypeError,
                "last element in __gproperties__ value tuple must be an int");
            ret = FALSE;
            break;
        }
        if (!pygi_gint_from_py (item, &flags)) {
            ret = FALSE;
            break;
        }

        /* slice is the extra items in the tuple */
        slice = PySequence_GetSlice (value, 3, val_length - 1);
        pspec =
            create_property (prop_name, prop_type, nick, blurb, slice, flags);
        Py_DECREF (slice);

        if (pspec) {
            g_object_class_install_property (klass, 1, pspec);
        } else {
            PyObject *type, *pvalue, *traceback;
            ret = FALSE;
            PyErr_Fetch (&type, &pvalue, &traceback);
            if (PyUnicode_Check (pvalue)) {
                char msg[256];
                g_snprintf (
                    msg, 256,
                    "%s (while registering property '%s' for GType '%s')",
                    PyUnicode_AsUTF8 (pvalue), prop_name,
                    G_OBJECT_CLASS_NAME (klass));
                Py_DECREF (pvalue);
                value = PyUnicode_FromString (msg);
            }
            PyErr_Restore (type, pvalue, traceback);
            break;
        }
    }

    return ret;
}

static void
pyg_object_get_property (GObject *object, guint property_id, GValue *value,
                         GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();

    object_wrapper = g_object_get_qdata (object, pygobject_wrapper_key);

    if (object_wrapper)
        Py_INCREF (object_wrapper);
    else
        object_wrapper = pygobject_new (object);

    if (object_wrapper == NULL) {
        PyGILState_Release (state);
        return;
    }

    retval = pygi_call_do_get_property (object_wrapper, pspec);
    if (retval && pyg_value_from_pyobject (value, retval) < 0) {
        PyErr_Print ();
    }
    Py_DECREF (object_wrapper);
    Py_XDECREF (retval);

    PyGILState_Release (state);
}

static void
pyg_object_set_property (GObject *object, guint property_id,
                         const GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec, *py_value;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();

    object_wrapper = g_object_get_qdata (object, pygobject_wrapper_key);

    if (object_wrapper)
        Py_INCREF (object_wrapper);
    else
        object_wrapper = pygobject_new (object);

    if (object_wrapper == NULL) {
        PyGILState_Release (state);
        return;
    }

    py_pspec = pygi_fundamental_new (pspec);
    py_value = pyg_value_as_pyobject (value, TRUE);

    retval = PyObject_CallMethod (object_wrapper, "do_set_property", "OO",
                                  py_pspec, py_value);
    if (retval) {
        Py_DECREF (retval);
    } else {
        PyErr_Print ();
    }

    Py_DECREF (object_wrapper);
    Py_DECREF (py_pspec);
    Py_DECREF (py_value);

    PyGILState_Release (state);
}

static void
pyg_object_dispose (GObject *object)
{
    PyObject *object_wrapper, *retval;
    GObjectClass *parent_class;
    PyGILState_STATE state = PyGILState_Ensure ();

    object_wrapper = g_object_get_qdata (object, pygobject_wrapper_key);
    Py_XINCREF (object_wrapper);

    if (object_wrapper != NULL
        && PyObject_HasAttrString (object_wrapper, "do_dispose")) {
        retval = PyObject_CallMethod (object_wrapper, "do_dispose", NULL);
        if (retval)
            Py_DECREF (retval);
        else
            PyErr_Print ();
    }
    Py_XDECREF (object_wrapper);

    PyGILState_Release (state);

    /* Find the first non-pygobject dispose method. */
    parent_class =
        g_type_class_peek (g_type_parent (G_TYPE_FROM_INSTANCE (object)));
    while (parent_class && parent_class->dispose == pyg_object_dispose) {
        parent_class = g_type_class_peek (
            g_type_parent (G_TYPE_FROM_CLASS (parent_class)));
    }

    if (parent_class && parent_class->dispose) {
        parent_class->dispose (object);
    }
}

void
pygobject__g_class_init (GObjectClass *class, PyObject *py_class)
{
    PyObject *gproperties, *gsignals, *overridden_signals;
    PyObject *class_dict = ((PyTypeObject *)py_class)->tp_dict;

    class->set_property = pyg_object_set_property;
    class->get_property = pyg_object_get_property;
    class->dispose = pyg_object_dispose;

    /* install signals */
    /* we look this up in the instance dictionary, so we don't
     * accidentally get a parent type's __gsignals__ attribute. */
    gsignals = PyDict_GetItemString (class_dict, "__gsignals__");
    if (gsignals) {
        if (!PyDict_Check (gsignals)) {
            PyErr_SetString (PyExc_TypeError,
                             "__gsignals__ attribute not a dict!");
            return;
        }
        if (!(overridden_signals = add_signals (class, gsignals))) {
            return;
        }
        if (PyDict_SetItemString (class_dict, "__gsignals__",
                                  overridden_signals)) {
            return;
        }
        Py_DECREF (overridden_signals);

        PyDict_DelItemString (class_dict, "__gsignals__");
    } else {
        PyErr_Clear ();
    }

    /* install properties */
    /* we look this up in the instance dictionary, so we don't
     * accidentally get a parent type's __gproperties__ attribute. */
    gproperties = PyDict_GetItemString (class_dict, "__gproperties__");
    if (gproperties) {
        if (!PyDict_Check (gproperties)) {
            PyErr_SetString (PyExc_TypeError,
                             "__gproperties__ attribute not a dict!");
            return;
        }
        if (!add_properties (class, gproperties)) {
            return;
        }
        PyDict_DelItemString (class_dict, "__gproperties__");
        /* Borrowed reference. Py_DECREF(gproperties); */
    } else {
        PyErr_Clear ();
    }
}
