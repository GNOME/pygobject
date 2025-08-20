/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   gimodule.c: wrapper for the gobject-introspection library.
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

#include "config.h"
#include "gimodule.h"
#include "pygboxed.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pygi-async.h"
#include "pygi-basictype.h"
#include "pygi-boxed.h"
#include "pygi-ccallback.h"
#include "pygi-closure.h"
#include "pygi-error.h"
#include "pygi-foreign.h"
#include "pygi-fundamental.h"
#include "pygi-info.h"
#include "pygi-property.h"
#include "pygi-repository.h"
#include "pygi-resulttuple.h"
#include "pygi-source.h"
#include "pygi-struct.h"
#include "pygi-type.h"
#include "pygi-util.h"
#include "pygi-value.h"
#include "pyginterface.h"
#include "pygobject-internal.h"
#include "pygobject-object.h"
#include "pygoptioncontext.h"
#include "pygoptiongroup.h"
#include "pygpointer.h"
#include "pygspawn.h"

PyObject *PyGIWarning;
PyObject *PyGIDeprecationWarning;
PyObject *_PyGIDefaultArgPlaceholder;

static int _gi_exec (PyObject *module);

static void pyg_flags_add_constants (PyObject *module, GType flags_type,
                                     const gchar *strip_prefix);

/**
 * pyg_enum_add_constants:
 * @module: a Python module
 * @enum_type: the GType of the enumeration.
 * @strip_prefix: the prefix to strip from the constant names.
 *
 * Adds constants to the given Python module for each value name of
 * the enumeration.  A prefix will be stripped from each enum name.
 */
static void
pyg_enum_add_constants (PyObject *module, GType enum_type,
                        const gchar *strip_prefix)
{
    GEnumClass *eclass;
    guint i;

    if (!G_TYPE_IS_ENUM (enum_type)) {
        if (G_TYPE_IS_FLAGS (enum_type)) /* See bug #136204 */
            pyg_flags_add_constants (module, enum_type, strip_prefix);
        else
            g_warning ("`%s' is not an enum type", g_type_name (enum_type));
        return;
    }
    g_return_if_fail (strip_prefix != NULL);

    eclass = G_ENUM_CLASS (g_type_class_ref (enum_type));

    for (i = 0; i < eclass->n_values; i++) {
        const gchar *name = eclass->values[i].value_name;
        gint value = eclass->values[i].value;

        PyModule_AddIntConstant (
            module, (char *)pyg_constant_strip_prefix (name, strip_prefix),
            (long)value);
    }

    g_type_class_unref (eclass);
}

/**
 * pyg_flags_add_constants:
 * @module: a Python module
 * @flags_type: the GType of the flags type.
 * @strip_prefix: the prefix to strip from the constant names.
 *
 * Adds constants to the given Python module for each value name of
 * the flags set.  A prefix will be stripped from each flag name.
 */
static void
pyg_flags_add_constants (PyObject *module, GType flags_type,
                         const gchar *strip_prefix)
{
    GFlagsClass *fclass;
    guint i;

    if (!G_TYPE_IS_FLAGS (flags_type)) {
        if (G_TYPE_IS_ENUM (flags_type)) /* See bug #136204 */
            pyg_enum_add_constants (module, flags_type, strip_prefix);
        else
            g_warning ("`%s' is not an flags type", g_type_name (flags_type));
        return;
    }
    g_return_if_fail (strip_prefix != NULL);

    fclass = G_FLAGS_CLASS (g_type_class_ref (flags_type));

    for (i = 0; i < fclass->n_values; i++) {
        const gchar *name = fclass->values[i].value_name;
        guint value = fclass->values[i].value;

        PyModule_AddIntConstant (
            module, (char *)pyg_constant_strip_prefix (name, strip_prefix),
            (long)value);
    }

    g_type_class_unref (fclass);
}

/**
 * pyg_set_thread_block_funcs:
 * Deprecated, only available for ABI compatibility.
 */
static void
_pyg_set_thread_block_funcs (PyGThreadBlockFunc block_threads_func,
                             PyGThreadBlockFunc unblock_threads_func)
{
    PyGILState_STATE state = PyGILState_Ensure ();
    PyErr_Warn (PyExc_DeprecationWarning,
                "Using pyg_set_thread_block_funcs is not longer needed. "
                "PyGObject always uses Py_BLOCK/UNBLOCK_THREADS.");
    PyGILState_Release (state);
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

static PyObject *
pyg_param_spec_new (GParamSpec *pspec)
{
    PyErr_SetString (PyExc_NotImplementedError,
                     "Creating ParamSpecs through pyg_param_spec_new is no "
                     "longer supported");
    return NULL;
}

static GParamSpec *
pyg_param_spec_from_object (PyObject *tuple)
{
    PyErr_SetString (PyExc_NotImplementedError,
                     "Creating ParamSpecs through pyg_param_spec_from_object "
                     "is no longer supported");
    return NULL;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * pyg_parse_constructor_args: helper function for PyGObject constructors
 * @obj_type: GType of the GObject, for parameter introspection
 * @arg_names: %NULL-terminated array of constructor argument names
 * @prop_names: %NULL-terminated array of property names, with direct
 * correspondence to @arg_names
 * @params: GParameter array where parameters will be placed; length
 * of this array must be at least equal to the number of
 * arguments/properties
 * @nparams: output parameter to contain actual number of arguments found
 * @py_args: array of PyObject* containing the actual constructor arguments
 *
 * Parses an array of PyObject's and creates a GParameter array
 *
 * Return value: %TRUE if all is successful, otherwise %FALSE and
 * python exception set.
 **/
static gboolean
pyg_parse_constructor_args (GType obj_type, char **arg_names,
                            char **prop_names, GParameter *params,
                            guint *nparams, PyObject **py_args)
{
    guint arg_i, param_i;
    GObjectClass *oclass;

    oclass = g_type_class_ref (obj_type);
    g_return_val_if_fail (oclass, FALSE);

    for (param_i = arg_i = 0; arg_names[arg_i]; ++arg_i) {
        GParamSpec *spec;
        if (!py_args[arg_i]) continue;
        spec = g_object_class_find_property (oclass, prop_names[arg_i]);
        params[param_i].name = prop_names[arg_i];
        g_value_init (&params[param_i].value, spec->value_type);
        if (pyg_value_from_pyobject (&params[param_i].value, py_args[arg_i])
            == -1) {
            guint i;
            PyErr_Format (PyExc_TypeError,
                          "could not convert parameter '%s' of type '%s'",
                          arg_names[arg_i], g_type_name (spec->value_type));
            g_type_class_unref (oclass);
            for (i = 0; i < param_i; ++i) g_value_unset (&params[i].value);
            return FALSE;
        }
        ++param_i;
    }
    g_type_class_unref (oclass);
    *nparams = param_i;
    return TRUE;
}

G_GNUC_END_IGNORE_DEPRECATIONS

/* Only for backwards compatibility */
static int
pygobject_enable_threads (void)
{
    return 0;
}

static int
pygobject_gil_state_ensure (void)
{
    return PyGILState_Ensure ();
}

static void
pygobject_gil_state_release (int flag)
{
    PyGILState_Release (flag);
}

static void
pyg_register_class_init (GType gtype, PyGClassInitFunc class_init)
{
    GSList *list;

    list = g_type_get_qdata (gtype, pygobject_class_init_key);
    list = g_slist_prepend (list, class_init);
    g_type_set_qdata (gtype, pygobject_class_init_key, list);
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

static void
pyg_object_class_init (GObjectClass *class, PyObject *py_class)
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

static GPrivate pygobject_construction_wrapper;

static inline void
pygobject_init_wrapper_set (PyObject *wrapper)
{
    g_private_set (&pygobject_construction_wrapper, wrapper);
}

static inline PyObject *
pygobject_init_wrapper_get (void)
{
    return (PyObject *)g_private_get (&pygobject_construction_wrapper);
}

int
pygobject_constructv (PyGObject *self, guint n_properties, const char *names[],
                      const GValue values[])
{
    GObject *obj;
    GType type;

    g_assert (self->obj == NULL);
    pygobject_init_wrapper_set ((PyObject *)self);

    type = pyg_type_from_object ((PyObject *)self);
    obj = g_object_new_with_properties (type, n_properties, names, values);

    if (G_IS_INITIALLY_UNOWNED (obj)) {
        g_object_ref_sink (obj);
    }

    pygobject_init_wrapper_set (NULL);
    self->obj = obj;
    pygobject_register_wrapper ((PyObject *)self);

    return 0;
}

static void
pygobject__g_instance_init (GTypeInstance *instance, gpointer g_class)
{
    GObject *object;
    PyObject *wrapper, *result;
    PyGILState_STATE state;
    gboolean needs_init = FALSE;

    g_return_if_fail (G_IS_OBJECT (instance));

    object = (GObject *)instance;

    wrapper = g_object_get_qdata (object, pygobject_wrapper_key);
    if (wrapper == NULL) {
        wrapper = pygobject_init_wrapper_get ();
        if (wrapper && ((PyGObject *)wrapper)->obj == NULL) {
            ((PyGObject *)wrapper)->obj = object;
            pygobject_register_wrapper (wrapper);
        }
    }
    pygobject_init_wrapper_set (NULL);

    state = PyGILState_Ensure ();

    if (wrapper == NULL) {
        /* this looks like a python object created through
           * g_object_new -> we have no python wrapper, so create it
           * now */

        if (g_object_is_floating (object)) {
            g_object_ref (object);
            wrapper = pygobject_new_full (object,
                                          /*steal=*/TRUE, g_class);
            g_object_force_floating (object);
        } else {
            wrapper = pygobject_new_full (object,
                                          /*steal=*/FALSE, g_class);
        }

        needs_init = TRUE;
    }

    /* XXX: used for Gtk.Template */
    gboolean is_final_subclass = G_OBJECT_TYPE (object)
                                 == G_OBJECT_CLASS_TYPE (g_class);
    if (is_final_subclass
        && PyObject_HasAttrString ((PyObject *)Py_TYPE (wrapper),
                                   "__dontuse_ginstance_init__")) {
        result =
            PyObject_CallMethod (wrapper, "__dontuse_ginstance_init__", NULL);
        if (result == NULL)
            PyErr_Print ();
        else
            Py_DECREF (result);
    }

    if (needs_init) {
        result = PyObject_CallMethod (wrapper, "__init__", NULL);
        if (result == NULL)
            PyErr_Print ();
        else
            Py_DECREF (result);
    }

    PyGILState_Release (state);
}

/*  This implementation is bad, see bug 566571 for an example why.
 *  Instead of scanning explicitly declared bases for interfaces, we
 *  should automatically initialize all implemented interfaces to
 *  prevent bugs like that one.  However, this will lead to
 *  performance degradation as each virtual method in derived classes
 *  will round-trip through do_*() stuff, *even* if it is not
 *  overriden.  We need to teach codegen to retain parent method
 *  instead of setting virtual to *_proxy_do_*() if corresponding
 *  do_*() is not overriden.  Ok, that was a messy explanation.
 */
static void
pyg_type_add_interfaces (PyTypeObject *class, GType instance_type,
                         PyObject *bases, GType *parent_interfaces,
                         guint n_parent_interfaces)
{
    int i;

    if (!bases) {
        g_warning ("type has no bases");
        return;
    }

    for (i = 0; i < PyTuple_GET_SIZE (bases); ++i) {
        PyObject *base = PyTuple_GET_ITEM (bases, i);
        GType itype;
        const GInterfaceInfo *iinfo;
        GInterfaceInfo iinfo_copy;

        /* 'base' can also be a PyClassObject, see bug #566571. */
        if (!PyType_Check (base)) continue;

        if (!PyType_IsSubtype ((PyTypeObject *)base, &PyGInterface_Type))
            continue;

        itype = pyg_type_from_object (base);

        /* Happens for _implementations_ of an interface. */
        if (!G_TYPE_IS_INTERFACE (itype)) continue;

        iinfo = pyg_lookup_interface_info (itype);
        if (!iinfo) {
            gchar *error;
            error = g_strdup_printf (
                "Interface type %s "
                "has no Python implementation support",
                ((PyTypeObject *)base)->tp_name);
            PyErr_Warn (PyExc_RuntimeWarning, error);
            g_free (error);
            continue;
        }

        iinfo_copy = *iinfo;
        iinfo_copy.interface_data = class;
        g_type_add_interface_static (instance_type, itype, &iinfo_copy);
    }
}

static int
pyg_run_class_init (GType gtype, gpointer gclass, PyTypeObject *pyclass)
{
    GSList *list;
    PyGClassInitFunc class_init;
    GType parent_type;
    int rv;

    parent_type = g_type_parent (gtype);
    if (parent_type) {
        rv = pyg_run_class_init (parent_type, gclass, pyclass);
        if (rv) return rv;
    }

    list = g_type_get_qdata (gtype, pygobject_class_init_key);
    for (; list; list = list->next) {
        class_init = list->data;
        rv = class_init (gclass, pyclass);
        if (rv) return rv;
    }

    return 0;
}

static char *
get_type_name_for_class (PyTypeObject *class)
{
    gint i, name_serial;
    char name_serial_str[16];
    PyObject *module;
    char *type_name = NULL;

    /* make name for new GType */
    name_serial = 1;
    /* give up after 1000 tries, just in case.. */
    while (name_serial < 1000) {
        g_free (type_name);
        g_snprintf (name_serial_str, 16, "-v%i", name_serial);
        module = PyObject_GetAttrString ((PyObject *)class, "__module__");
        if (module && PyUnicode_Check (module)) {
            type_name =
                g_strconcat (PyUnicode_AsUTF8 (module), ".", class->tp_name,
                             name_serial > 1 ? name_serial_str : NULL, NULL);
            Py_DECREF (module);
        } else {
            if (module)
                Py_DECREF (module);
            else
                PyErr_Clear ();
            type_name = g_strconcat (class->tp_name,
                                     name_serial > 1 ? name_serial_str : NULL,
                                     NULL);
        }
        /* convert '.' in type name to '+', which isn't banned (grumble) */
        for (i = 0; type_name[i] != '\0'; i++)
            if (type_name[i] == '.') type_name[i] = '+';
        if (g_type_from_name (type_name) == 0)
            break; /* we now have a unique name */
        ++name_serial;
    }

    return type_name;
}

static int
pyg_type_register (PyTypeObject *class, const char *type_name)
{
    PyObject *gtype;
    GType parent_type, instance_type;
    GType *parent_interfaces;
    guint n_parent_interfaces;
    GTypeQuery query;
    gpointer gclass;
    GTypeInfo type_info = {
        0, /* class_size */

        (GBaseInitFunc)NULL,
        (GBaseFinalizeFunc)NULL,

        (GClassInitFunc)NULL,
        (GClassFinalizeFunc)NULL,
        NULL, /* class_data */

        0,    /* instance_size */
        0,    /* n_preallocs */
        (GInstanceInitFunc)NULL,
    };
    gchar *new_type_name;

    if (PyType_IsSubtype (class, &PyGObject_Type)) {
        type_info.class_init = (GClassInitFunc)pyg_object_class_init;
        type_info.instance_init = pygobject__g_instance_init;
    }

    /* find the GType of the parent */
    parent_type = pyg_type_from_object ((PyObject *)class);
    if (!parent_type) return -1;

    parent_interfaces = g_type_interfaces (parent_type, &n_parent_interfaces);

    if (type_name) /* care is taken below not to free this */
        new_type_name = (gchar *)type_name;
    else
        new_type_name = get_type_name_for_class (class);

    /* set class_data that will be passed to the class_init function. */
    type_info.class_data = class;

    /* fill in missing values of GTypeInfo struct */
    g_type_query (parent_type, &query);
    type_info.class_size = (guint16)query.class_size;
    type_info.instance_size = (guint16)query.instance_size;

    /* create new typecode */
    instance_type =
        g_type_register_static (parent_type, new_type_name, &type_info, 0);
    if (instance_type == 0) {
        PyErr_Format (PyExc_RuntimeError,
                      "could not create new GType: %s (subclass of %s)",
                      new_type_name, g_type_name (parent_type));

        if (type_name == NULL) g_free (new_type_name);

        return -1;
    }

    if (type_name == NULL) g_free (new_type_name);

    /* store pointer to the class with the GType */
    Py_INCREF (class);
    g_type_set_qdata (instance_type, pygobject_class_key, class);

    /* Mark this GType as a custom python type */
    g_type_set_qdata (instance_type, pygobject_custom_key,
                      GINT_TO_POINTER (1));

    /* set new value of __gtype__ on class */
    gtype = pyg_type_wrapper_new (instance_type);
    PyObject_SetAttrString ((PyObject *)class, "__gtype__", gtype);
    Py_DECREF (gtype);

    /* if no __doc__, set it to the auto doc descriptor */
    if (PyDict_GetItemString (class->tp_dict, "__doc__") == NULL) {
        PyDict_SetItemString (class->tp_dict, "__doc__",
                              pyg_object_descr_doc_get ());
    }

    /*
     * Note, all interfaces need to be registered before the first
     * g_type_class_ref(), see bug #686149.
     *
     * See also comment above pyg_type_add_interfaces().
     */
    pyg_type_add_interfaces (class, instance_type, class->tp_bases,
                             parent_interfaces, n_parent_interfaces);


    gclass = g_type_class_ref (instance_type);
    if (PyErr_Occurred () != NULL) {
        g_type_class_unref (gclass);
        g_free (parent_interfaces);
        return -1;
    }

    if (pyg_run_class_init (instance_type, gclass, class)) {
        g_type_class_unref (gclass);
        g_free (parent_interfaces);
        return -1;
    }
    g_type_class_unref (gclass);
    g_free (parent_interfaces);

    if (PyErr_Occurred () != NULL) return -1;
    return 0;
}

static PyObject *
_wrap_pyg_type_register (PyObject *self, PyObject *args)
{
    PyTypeObject *class;
    char *type_name = NULL;

    if (!PyArg_ParseTuple (args, "O!|z:gobject.type_register", &PyType_Type,
                           &class, &type_name))
        return NULL;

    GType base_gtype = pyg_type_from_object ((PyObject *)class->tp_base);

    if (base_gtype == G_TYPE_INVALID) {
        PyErr_SetString (PyExc_TypeError,
                         "argument must be a Fundamental or GObject subclass");
        return NULL;
    }

    /* Check if type already registered */
    if (pyg_type_from_object ((PyObject *)class) == base_gtype) {
        if (pyg_type_register (class, type_name)) return NULL;
    }

    return Py_NewRef (class);
}

static PyObject *
_wrap_pyg_enum_register (PyObject *self, PyObject *args)
{
    PyTypeObject *class;
    const char *type_name = NULL;
    char *new_type_name;

    if (!PyArg_ParseTuple (args, "O!z:enum_register", &PyType_Type, &class,
                           &type_name))
        return NULL;

    if (!PyObject_IsSubclass ((PyObject *)class, (PyObject *)PyGEnum_Type)) {
        PyErr_SetString (PyExc_TypeError, "class is not a GEnum");
        return NULL;
    }

    if (type_name)
        new_type_name = g_strdup (type_name);
    else
        new_type_name = get_type_name_for_class (class);

    if (!pyg_enum_register (class, new_type_name)) return NULL;

    Py_RETURN_NONE;
}

static PyObject *
_wrap_pyg_flags_register (PyObject *self, PyObject *args)
{
    PyTypeObject *class;
    const char *type_name = NULL;
    char *new_type_name;

    if (!PyArg_ParseTuple (args, "O!z:flags_register", &PyType_Type, &class,
                           &type_name))
        return NULL;

    if (!PyObject_IsSubclass ((PyObject *)class, (PyObject *)PyGFlags_Type)) {
        PyErr_SetString (PyExc_TypeError, "class is not a GFlags");
        return NULL;
    }

    if (type_name)
        new_type_name = g_strdup (type_name);
    else
        new_type_name = get_type_name_for_class (class);

    if (!pyg_flags_register (class, new_type_name)) return NULL;

    Py_RETURN_NONE;
}

static GHashTable *log_handlers = NULL;
static gboolean log_handlers_disabled = FALSE;

static void
remove_handler (gpointer domain, gpointer handler, gpointer unused)
{
    g_log_remove_handler (domain, GPOINTER_TO_UINT (handler));
}

static void
_log_func (const gchar *log_domain, GLogLevelFlags log_level,
           const gchar *message, gpointer user_data)
{
    if (G_LIKELY (Py_IsInitialized ())) {
        PyGILState_STATE state;
        PyObject *warning = user_data;

        state = PyGILState_Ensure ();
        PyErr_Warn (warning, (char *)message);
        PyGILState_Release (state);
    } else
        g_log_default_handler (log_domain, log_level, message, user_data);
}

static void
add_warning_redirection (const char *domain, PyObject *warning)
{
    g_return_if_fail (domain != NULL);
    g_return_if_fail (warning != NULL);

    if (!log_handlers_disabled) {
        guint handler;
        gpointer old_handler;

        if (!log_handlers)
            log_handlers =
                g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

        if ((old_handler = g_hash_table_lookup (log_handlers, domain)))
            g_log_remove_handler (domain, GPOINTER_TO_UINT (old_handler));

        handler = g_log_set_handler (
            domain, G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING, _log_func,
            warning);
        g_hash_table_insert (log_handlers, g_strdup (domain),
                             GUINT_TO_POINTER (handler));
    }
}

static void
disable_warning_redirections (void)
{
    log_handlers_disabled = TRUE;

    if (log_handlers) {
        g_hash_table_foreach (log_handlers, remove_handler, NULL);
        g_hash_table_destroy (log_handlers);
        log_handlers = NULL;
    }
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
static int
pygi_register_warnings (PyObject *d)
{
    PyObject *warning;

    warning = PyErr_NewException ("gobject.Warning", PyExc_Warning, NULL);
    if (warning == NULL) return -1;
    PyDict_SetItemString (d, "Warning", warning);
    add_warning_redirection ("GLib", warning);
    add_warning_redirection ("GLib-GObject", warning);
    add_warning_redirection ("GThread", warning);

    return 0;
}

static PyObject *
_wrap_pyg_enum_add (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "module", "name", "g_type", "enum_info", NULL };
    PyObject *module, *py_g_type;
    PyGIBaseInfo *py_info;
    const char *name;
    GType g_type;
    GIEnumInfo *info;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "OsOO:enum_add", kwlist,
                                      &module, &name, &py_g_type, &py_info)) {
        return NULL;
    }

    if (!Py_IsNone (module) && !PyModule_Check (module)) {
        PyErr_SetString (PyExc_TypeError,
                         "first argument must be module or None");
        return NULL;
    }

    g_type = pyg_type_from_object (py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    if (Py_TYPE (py_info) == &PyGIEnumInfo_Type) {
        info = GI_ENUM_INFO (py_info->info);
    } else {
        info = NULL;
    }

    return pyg_enum_add_full (module, name, g_type, info);
}

static PyObject *
_wrap_pyg_flags_add (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "module", "name", "g_type", "flags_info", NULL };
    PyObject *module, *py_g_type;
    PyGIBaseInfo *py_info;
    const char *name;
    GType g_type;
    GIFlagsInfo *info;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "OsOO:flags_add", kwlist,
                                      &module, &name, &py_g_type, &py_info)) {
        return NULL;
    }

    if (!Py_IsNone (module) && !PyModule_Check (module)) {
        PyErr_SetString (PyExc_TypeError,
                         "first argument must be module or None");
        return NULL;
    }

    g_type = pyg_type_from_object (py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    if (Py_TYPE (py_info) == &PyGIEnumInfo_Type
        && GI_IS_FLAGS_INFO (py_info->info)) {
        info = GI_FLAGS_INFO (py_info->info);
    } else {
        info = NULL;
    }

    return pyg_flags_add_full (module, name, g_type, info);
}

static void
initialize_interface (GTypeInterface *iface, PyTypeObject *pytype)
{
    /* pygobject prints a warning if interface_init is NULL */
}

static PyObject *
_wrap_pyg_register_interface_info (PyObject *self, PyObject *args)
{
    PyObject *py_g_type;
    GType g_type;
    GInterfaceInfo *info;

    if (!PyArg_ParseTuple (args, "O!:register_interface_info",
                           &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object (py_g_type);
    if (!g_type_is_a (g_type, G_TYPE_INTERFACE)) {
        PyErr_SetString (PyExc_TypeError, "must be an interface");
        return NULL;
    }

    info = g_new0 (GInterfaceInfo, 1);
    info->interface_init = (GInterfaceInitFunc)initialize_interface;

    pyg_register_interface_info (g_type, info);
    g_free (info);

    Py_RETURN_NONE;
}

static void
find_vfunc_info (GIBaseInfo *vfunc_info, GType implementor_gtype,
                 gpointer *implementor_class_ret,
                 gpointer *implementor_vtable_ret,
                 GIFieldInfo **field_info_ret)
{
    GType ancestor_g_type = 0;
    GIBaseInfo *ancestor_info;
    GIStructInfo *struct_info;
    gpointer implementor_class = NULL;
    gboolean is_interface = FALSE;
    GIFieldInfo *field_info;

    ancestor_info = gi_base_info_get_container (vfunc_info);
    is_interface = GI_IS_INTERFACE_INFO (ancestor_info);

    ancestor_g_type = gi_registered_type_info_get_g_type (
        (GIRegisteredTypeInfo *)ancestor_info);
    implementor_class = g_type_class_ref (implementor_gtype);
    if (is_interface) {
        GTypeInstance *implementor_iface_class;
        implementor_iface_class =
            g_type_interface_peek (implementor_class, ancestor_g_type);
        if (implementor_iface_class == NULL) {
            g_type_class_unref (implementor_class);
            PyErr_Format (
                PyExc_RuntimeError,
                "Couldn't find GType of implementor of interface %s. "
                "Forgot to set __gtype_name__?",
                g_type_name (ancestor_g_type));
            return;
        }

        *implementor_vtable_ret = implementor_iface_class;

        struct_info = gi_interface_info_get_iface_struct (
            (GIInterfaceInfo *)ancestor_info);
    } else {
        struct_info =
            gi_object_info_get_class_struct ((GIObjectInfo *)ancestor_info);
        *implementor_vtable_ret = implementor_class;
    }

    *implementor_class_ret = implementor_class;

    field_info = gi_struct_info_find_field (
        struct_info, gi_base_info_get_name ((GIBaseInfo *)vfunc_info));
    if (field_info != NULL) {
        GITypeInfo *type_info;

        type_info = gi_field_info_get_type_info (field_info);
        if (gi_type_info_get_tag (type_info) == GI_TYPE_TAG_INTERFACE) {
            *field_info_ret = field_info;
        } else {
            gi_base_info_unref (field_info);
        }
        gi_base_info_unref (type_info);
    }

    gi_base_info_unref (struct_info);
}

static PyObject *
_wrap_pyg_hook_up_vfunc_implementation (PyObject *self, PyObject *args)
{
    PyGIBaseInfo *py_info;
    PyObject *py_type;
    PyObject *py_function;
    GType implementor_gtype = 0;
    gpointer implementor_class = NULL;
    gpointer implementor_vtable = NULL;
    GIFieldInfo *field_info = NULL;
    gpointer *method_ptr = NULL;
    PyGICClosure *closure = NULL;
    PyGIClosureCache *cache = NULL;

    if (!PyArg_ParseTuple (args, "O!O!O:hook_up_vfunc_implementation",
                           &PyGIBaseInfo_Type, &py_info, &PyGTypeWrapper_Type,
                           &py_type, &py_function))
        return NULL;

    implementor_gtype = pyg_type_from_object (py_type);
    g_assert (G_TYPE_IS_CLASSED (implementor_gtype));

    find_vfunc_info (py_info->info, implementor_gtype, &implementor_class,
                     &implementor_vtable, &field_info);
    if (field_info != NULL) {
        GITypeInfo *type_info;
        GICallableInfo *callable_info;
        gint offset;

        type_info = gi_field_info_get_type_info (field_info);
        callable_info =
            GI_CALLABLE_INFO (gi_type_info_get_interface (type_info));
        offset = gi_field_info_get_offset (field_info);
        method_ptr = G_STRUCT_MEMBER_P (implementor_vtable, offset);

        cache = pygi_closure_cache_new (callable_info);
        closure = _pygi_make_native_closure ((GICallableInfo *)callable_info,
                                             cache, GI_SCOPE_TYPE_NOTIFIED,
                                             py_function, NULL);

        *method_ptr = gi_callable_info_get_closure_native_address (
            callable_info, closure->closure);

        gi_base_info_unref (callable_info);
        gi_base_info_unref (type_info);
        gi_base_info_unref (field_info);
    }
    g_type_class_unref (implementor_class);

    Py_RETURN_NONE;
}

#if 0
/* Not used, left around for future reference */
static PyObject *
_wrap_pyg_has_vfunc_implementation (PyObject *self, PyObject *args)
{
    PyGIBaseInfo *py_info;
    PyObject *py_type;
    PyObject *py_ret;
    gpointer implementor_class = NULL;
    gpointer implementor_vtable = NULL;
    GType implementor_gtype = 0;
    GIFieldInfo *field_info = NULL;

    if (!PyArg_ParseTuple (args, "O!O!:has_vfunc_implementation",
                           &PyGIBaseInfo_Type, &py_info,
                           &PyGTypeWrapper_Type, &py_type))
        return NULL;

    implementor_gtype = pyg_type_from_object (py_type);
    g_assert (G_TYPE_IS_CLASSED (implementor_gtype));

    py_ret = Py_False;
    find_vfunc_info (py_info->info, implementor_gtype, &implementor_class, &implementor_vtable, &field_info);
    if (field_info != NULL) {
        gpointer *method_ptr;
        gint offset;

        offset = gi_field_info_get_offset (field_info);
        method_ptr = G_STRUCT_MEMBER_P (implementor_vtable, offset);
        if (*method_ptr != NULL) {
            py_ret = Py_True;
        }

        gi_base_info_unref (field_info);
    }
    g_type_class_unref (implementor_class);

    return Py_NewRef(py_ret);
}
#endif

static PyObject *
_wrap_pyg_variant_type_from_string (PyObject *self, PyObject *args)
{
    char *type_string;
    PyObject *py_type;
    PyObject *py_variant = NULL;

    if (!PyArg_ParseTuple (args, "s:variant_type_from_string", &type_string)) {
        return NULL;
    }

    py_type = pygi_type_import_by_name ("GLib", "VariantType");

    py_variant =
        pygi_boxed_new ((PyTypeObject *)py_type, type_string, FALSE, 0);

    return py_variant;
}

#define CHUNK_SIZE 8192

static PyObject *
pyg_channel_read (PyObject *self, PyObject *args, PyObject *kwargs)
{
    int max_count = -1;
    PyObject *py_iochannel, *ret_obj = NULL;
    gsize total_read = 0;
    GError *error = NULL;
    GIOStatus status = G_IO_STATUS_NORMAL;
    GIOChannel *iochannel = NULL;

    if (!PyArg_ParseTuple (args, "Oi:pyg_channel_read", &py_iochannel,
                           &max_count)) {
        return NULL;
    }
    if (!pyg_boxed_check (py_iochannel, G_TYPE_IO_CHANNEL)) {
        PyErr_SetString (PyExc_TypeError,
                         "first argument is not a GLib.IOChannel");
        return NULL;
    }

    if (max_count == 0) return PyBytes_FromString ("");

    iochannel = pyg_boxed_get (py_iochannel, GIOChannel);

    while (status == G_IO_STATUS_NORMAL
           && (max_count == -1 || total_read < (gsize)max_count)) {
        gsize single_read;
        char *buf;
        gsize buf_size;

        if (max_count == -1)
            buf_size = CHUNK_SIZE;
        else {
            buf_size = max_count - total_read;
            if (buf_size > CHUNK_SIZE) buf_size = CHUNK_SIZE;
        }

        if (ret_obj == NULL) {
            ret_obj = PyBytes_FromStringAndSize ((char *)NULL, buf_size);
            if (ret_obj == NULL) goto failure;
        } else if (buf_size + total_read > (gsize)PyBytes_Size (ret_obj)) {
            if (_PyBytes_Resize (&ret_obj, buf_size + total_read) == -1)
                goto failure;
        }

        buf = PyBytes_AsString (ret_obj) + total_read;

        Py_BEGIN_ALLOW_THREADS;
        status = g_io_channel_read_chars (iochannel, buf, buf_size,
                                          &single_read, &error);
        Py_END_ALLOW_THREADS;

        if (pygi_error_check (&error)) goto failure;

        total_read += single_read;
    }

    if (total_read != (gsize)PyBytes_Size (ret_obj)) {
        if (_PyBytes_Resize (&ret_obj, total_read) == -1) goto failure;
    }

    return ret_obj;

failure:
    Py_XDECREF (ret_obj);
    return NULL;
}

static PyObject *
pyg_main_context_query (PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *py_main_context;
    int max_priority = -1, n_fds;
    GMainContext *context;
    int timeout_msec = 0;
    GPollFD *fds;
    int n_poll, read_fds, i;
    PyObject *py_out, *py_fds;

    if (!PyArg_ParseTuple (args, "Oi:pyg_main_context_query", &py_main_context,
                           &max_priority)) {
        return NULL;
    }

    if (!pyg_boxed_check (py_main_context, G_TYPE_MAIN_CONTEXT)) {
        PyErr_SetString (PyExc_TypeError,
                         "first argument is not a GLib.MainContext");
        return NULL;
    }

    context = pyg_boxed_get (py_main_context, GMainContext);

    n_fds = g_main_context_query (context, max_priority, NULL, NULL, 0);

    fds = g_new0 (GPollFD, n_fds);

    n_poll = g_main_context_query (context, max_priority, &timeout_msec, fds,
                                   n_fds);

    read_fds = MIN (n_fds, n_poll);
    py_fds = PyList_New (read_fds);
    for (i = 0; i < read_fds; i++) {
        PyObject *py_boxed =
            pygi_gboxed_new (G_TYPE_POLLFD, &fds[i], TRUE, TRUE);
        if (py_boxed == NULL) {
            Py_DECREF (py_fds);
            g_free (fds);
            return NULL;
        }
        PyList_SET_ITEM (py_fds, i, py_boxed);
    }
    g_free (fds);

    py_out = PyTuple_New (2);
    PyTuple_SET_ITEM (py_out, 0, PyLong_FromLong (timeout_msec));
    PyTuple_SET_ITEM (py_out, 1, py_fds);

    return py_out;
}

static gboolean
marshal_emission_hook (GSignalInvocationHint *ihint, guint n_param_values,
                       const GValue *param_values, gpointer user_data)
{
    PyGILState_STATE state;
    gboolean retval = FALSE;
    PyObject *func, *args;
    PyObject *retobj;
    PyObject *params;
    guint i;

    state = PyGILState_Ensure ();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New (n_param_values);

    for (i = 0; i < n_param_values; i++) {
        PyObject *item = pyg_value_as_pyobject (&param_values[i], FALSE);

        /* error condition */
        if (!item) {
            goto out;
        }
        PyTuple_SetItem (params, i, item);
    }

    args = (PyObject *)user_data;
    func = PyTuple_GetItem (args, 0);
    args = PySequence_Concat (params, PyTuple_GetItem (args, 1));
    Py_DECREF (params);

    /* params passed to function may have extra arguments */

    retobj = PyObject_CallObject (func, args);
    Py_DECREF (args);
    if (retobj == NULL) {
        PyErr_Print ();
    }

    retval = Py_IsTrue (retobj);
    Py_XDECREF (retobj);
out:
    PyGILState_Release (state);
    return retval;
}

/**
 * pyg_destroy_notify:
 * @user_data: a PyObject pointer.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call Py_DECREF on the data.
 */
static void
pyg_destroy_notify (gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    Py_DECREF (obj);
    PyGILState_Release (state);
}

static PyObject *
pyg_add_emission_hook (PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *data, *repr;
    gchar *name;
    gulong hook_id;
    guint sigid;
    Py_ssize_t len;
    GQuark detail = 0;
    GType gtype;
    PyObject *pygtype;

    len = PyTuple_Size (args);
    if (len < 3) {
        PyErr_SetString (
            PyExc_TypeError,
            "gobject.add_emission_hook requires at least 3 arguments");
        return NULL;
    }
    first = PySequence_GetSlice (args, 0, 3);
    if (!PyArg_ParseTuple (first, "OsO:add_emission_hook", &pygtype, &name,
                           &callback)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);

    if ((gtype = pyg_type_from_object (pygtype)) == 0) {
        return NULL;
    }
    if (!PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, "third argument must be callable");
        return NULL;
    }

    if (!g_signal_parse_name (name, gtype, &sigid, &detail, TRUE)) {
        repr = PyObject_Repr ((PyObject *)self);
        PyErr_Format (PyExc_TypeError, "%s: unknown signal name: %s",
                      PyUnicode_AsUTF8 (repr), name);
        Py_DECREF (repr);
        return NULL;
    }
    extra_args = PySequence_GetSlice (args, 3, len);
    if (extra_args == NULL) return NULL;

    data = Py_BuildValue ("(ON)", callback, extra_args);
    if (data == NULL) return NULL;

    hook_id = g_signal_add_emission_hook (sigid, detail, marshal_emission_hook,
                                          data,
                                          (GDestroyNotify)pyg_destroy_notify);

    return pygi_gulong_to_py (hook_id);
}

static PyObject *
pyg_signal_new (PyObject *self, PyObject *args)
{
    gchar *signal_name;
    PyObject *py_type;
    GSignalFlags signal_flags;
    GType return_type;
    PyObject *py_return_type, *py_param_types;

    GType instance_type = 0;
    Py_ssize_t py_n_params;
    guint n_params, i;
    GType *param_types;

    guint signal_id;

    if (!PyArg_ParseTuple (args, "sOiOO:gobject.signal_new", &signal_name,
                           &py_type, &signal_flags, &py_return_type,
                           &py_param_types))
        return NULL;

    instance_type = pyg_type_from_object (py_type);
    if (!instance_type) return NULL;
    if (!(G_TYPE_IS_INSTANTIATABLE (instance_type)
          || G_TYPE_IS_INTERFACE (instance_type))) {
        PyErr_SetString (
            PyExc_TypeError,
            "argument 2 must be an object type or interface type");
        return NULL;
    }

    return_type = pyg_type_from_object (py_return_type);
    if (!return_type) return NULL;

    if (!PySequence_Check (py_param_types)) {
        PyErr_SetString (PyExc_TypeError,
                         "argument 5 must be a sequence of GType codes");
        return NULL;
    }

    py_n_params = PySequence_Length (py_param_types);
    if (py_n_params < 0) return FALSE;

    if (!pygi_guint_from_pyssize (py_n_params, &n_params)) return FALSE;

    param_types = g_new (GType, n_params);
    for (i = 0; i < n_params; i++) {
        PyObject *item = PySequence_GetItem (py_param_types, i);

        param_types[i] = pyg_type_from_object (item);
        if (param_types[i] == 0) {
            PyErr_Clear ();
            Py_DECREF (item);
            PyErr_SetString (PyExc_TypeError,
                             "argument 5 must be a sequence of GType codes");
            g_free (param_types);
            return NULL;
        }
        Py_DECREF (item);
    }

    signal_id = g_signal_newv (
        signal_name, instance_type, signal_flags,
        pyg_signal_class_closure_get (), (GSignalAccumulator)0, NULL,
        (GSignalCMarshaller)0, return_type, n_params, param_types);
    g_free (param_types);
    if (signal_id != 0) return pygi_guint_to_py (signal_id);
    PyErr_SetString (PyExc_RuntimeError, "could not create signal");
    return NULL;
}

static PyObject *
pyg_object_class_list_properties (PyObject *self, PyObject *args)
{
    GParamSpec **specs;
    PyObject *py_itype, *list;
    GType itype;
    GObjectClass *class = NULL;
    gpointer iface = NULL;
    guint nprops;
    guint i;

    if (!PyArg_ParseTuple (args, "O:gobject.list_properties", &py_itype))
        return NULL;
    if ((itype = pyg_type_from_object (py_itype)) == 0) return NULL;

    if (G_TYPE_IS_INTERFACE (itype)) {
        iface = g_type_default_interface_ref (itype);
        if (!iface) {
            PyErr_SetString (PyExc_RuntimeError,
                             "could not get a reference to interface type");
            return NULL;
        }
        specs = g_object_interface_list_properties (iface, &nprops);
    } else if (g_type_is_a (itype, G_TYPE_OBJECT)) {
        class = g_type_class_ref (itype);
        if (!class) {
            PyErr_SetString (PyExc_RuntimeError,
                             "could not get a reference to type class");
            return NULL;
        }
        specs = g_object_class_list_properties (class, &nprops);
    } else {
        PyErr_SetString (PyExc_TypeError,
                         "type must be derived from GObject or an interface");
        return NULL;
    }

    list = PyTuple_New (nprops);
    if (list == NULL) {
        g_free (specs);
        g_type_class_unref (class);
        return NULL;
    }
    for (i = 0; i < nprops; i++) {
        PyTuple_SetItem (list, i, pygi_fundamental_new (specs[i]));
    }
    g_free (specs);
    if (class)
        g_type_class_unref (class);
    else
        g_type_default_interface_unref (iface);

    return list;
}

static PyObject *
pyg__install_metaclass (PyObject *dummy, PyTypeObject *metaclass)
{
    PyGObject_MetaType = (PyTypeObject *)Py_NewRef (metaclass);
    Py_SET_TYPE (&PyGObject_Type, (PyTypeObject *)Py_NewRef (metaclass));

    Py_RETURN_NONE;
}

static PyObject *
_wrap_pyig_pyos_getsig (PyObject *self, PyObject *args)
{
    int sig_num;

    if (!PyArg_ParseTuple (args, "i:pyos_getsig", &sig_num)) return NULL;

    return PyLong_FromVoidPtr ((void *)(PyOS_getsig (sig_num)));
}

static PyObject *
_wrap_pyig_pyos_setsig (PyObject *self, PyObject *args)
{
    int sig_num;
    PyObject *sig_handler;

    if (!PyArg_ParseTuple (args, "iO!:pyos_setsig", &sig_num, &PyLong_Type,
                           &sig_handler))
        return NULL;

    return PyLong_FromVoidPtr ((void *)(PyOS_setsig (
        sig_num, (PyOS_sighandler_t)PyLong_AsVoidPtr (sig_handler))));
}

static PyObject *
_wrap_pygobject_new_full (PyObject *self, PyObject *args)
{
    PyObject *ptr_value, *long_value;
    PyObject *steal;
    GObject *obj;

    if (!PyArg_ParseTuple (args, "OO", &ptr_value, &steal)) return NULL;

    long_value = PyNumber_Long (ptr_value);
    if (!long_value) {
        PyErr_SetString (PyExc_TypeError, "first argument must be an integer");
        return NULL;
    }
    obj = PyLong_AsVoidPtr (long_value);
    Py_DECREF (long_value);

    if (!G_IS_OBJECT (obj)) {
        PyErr_SetString (PyExc_TypeError, "pointer is not a GObject");
        return NULL;
    }

    return pygobject_new_full (obj, PyObject_IsTrue (steal), NULL);
}

static PyMethodDef _gi_functions[] = {
    { "pygobject_new_full", (PyCFunction)_wrap_pygobject_new_full,
      METH_VARARGS },
    { "enum_add", (PyCFunction)_wrap_pyg_enum_add,
      METH_VARARGS | METH_KEYWORDS },
    { "flags_add", (PyCFunction)_wrap_pyg_flags_add,
      METH_VARARGS | METH_KEYWORDS },

    { "register_interface_info",
      (PyCFunction)_wrap_pyg_register_interface_info, METH_VARARGS },
    { "hook_up_vfunc_implementation",
      (PyCFunction)_wrap_pyg_hook_up_vfunc_implementation, METH_VARARGS },
    { "variant_type_from_string",
      (PyCFunction)_wrap_pyg_variant_type_from_string, METH_VARARGS },
    { "source_new", (PyCFunction)pygi_source_new, METH_NOARGS },
    { "pyos_getsig", (PyCFunction)_wrap_pyig_pyos_getsig, METH_VARARGS },
    { "pyos_setsig", (PyCFunction)_wrap_pyig_pyos_setsig, METH_VARARGS },
    { "source_set_callback", (PyCFunction)pygi_source_set_callback,
      METH_VARARGS },
    { "io_channel_read", (PyCFunction)pyg_channel_read, METH_VARARGS },
    { "main_context_query", (PyCFunction)pyg_main_context_query,
      METH_VARARGS },
    { "require_foreign", (PyCFunction)pygi_require_foreign,
      METH_VARARGS | METH_KEYWORDS },
    { "register_foreign", (PyCFunction)pygi_register_foreign, METH_NOARGS },
    { "spawn_async", (PyCFunction)pyglib_spawn_async,
      METH_VARARGS | METH_KEYWORDS,
      "spawn_async(argv, envp=None, working_directory=None,\n"
      "            flags=0, child_setup=None, user_data=None,\n"
      "            standard_input=None, standard_output=None,\n"
      "            standard_error=None) -> (pid, stdin, stdout, stderr)\n"
      "\n"
      "Execute a child program asynchronously within a glib.MainLoop()\n"
      "See the reference manual for a complete reference.\n" },
    { "type_register", _wrap_pyg_type_register, METH_VARARGS },
    { "enum_register", _wrap_pyg_enum_register, METH_VARARGS },
    { "flags_register", _wrap_pyg_flags_register, METH_VARARGS },
    { "signal_new", pyg_signal_new, METH_VARARGS },
    { "list_properties", pyg_object_class_list_properties, METH_VARARGS },
    { "new", (PyCFunction)pyg_object_new, METH_VARARGS | METH_KEYWORDS },
    { "add_emission_hook", (PyCFunction)pyg_add_emission_hook, METH_VARARGS },
    { "_install_metaclass", (PyCFunction)pyg__install_metaclass, METH_O },
    { "_gvalue_get", (PyCFunction)pyg__gvalue_get, METH_O },
    { "_gvalue_get_type", (PyCFunction)pyg__gvalue_get_type, METH_O },
    { "_gvalue_set", (PyCFunction)pyg__gvalue_set, METH_VARARGS },
    { NULL, NULL, 0 },
};

static struct PyGI_API CAPI = {
    pygi_register_foreign_struct,
};

const gpointer PyGParamSpec_Type_Stub = NULL;

struct _PyGObject_Functions pygobject_api_functions = {
    pygobject_register_class,
    pygobject_register_wrapper,
    pygobject_lookup_class,
    pygobject_new,

    pyg_closure_new,
    pygobject_watch_closure,
    pyg_destroy_notify,

    pyg_type_from_object,
    pyg_type_wrapper_new,
    pyg_enum_get_value,
    pyg_flags_get_value,
    pyg_register_gtype_custom,
    pyg_value_from_pyobject,
    pyg_value_as_pyobject,

    pyg_register_interface,

    &PyGBoxed_Type,
    pygi_register_gboxed,
    pygi_gboxed_new,

    &PyGPointer_Type,
    pyg_register_pointer,
    pyg_pointer_new,

    pyg_enum_add_constants,
    pyg_flags_add_constants,

    pyg_constant_strip_prefix,

    pygi_error_check,

    _pyg_set_thread_block_funcs,
    (PyGThreadBlockFunc)0, /* block_threads */
    (PyGThreadBlockFunc)0, /* unblock_threads */

    (PyTypeObject *)&PyGParamSpec_Type_Stub,
    pyg_param_spec_new,
    pyg_param_spec_from_object,

    pyg_pyobj_to_unichar_conv,
    pyg_parse_constructor_args,
    pyg_param_gvalue_as_pyobject,
    pyg_param_gvalue_from_pyobject,

    NULL /* PyGEnum_Type */,
    pyg_enum_add,
    pyg_enum_from_gtype,

    NULL /* PyGFlags_Type */,
    pyg_flags_add,
    pyg_flags_from_gtype,

    TRUE, /* threads_enabled */

    pygobject_enable_threads,
    pygobject_gil_state_ensure,
    pygobject_gil_state_release,
    pyg_register_class_init,
    pyg_register_interface_info,

    pyg_closure_set_exception_handler,

    add_warning_redirection,
    disable_warning_redirections,

    NULL, /* previously type_register_custom */

    pygi_gerror_exception_check,

    pyg_option_group_new,
    pyg_type_from_object_strict,

    pygobject_new_full,
    &PyGObject_Type,

    pyg_value_from_pyobject_with_error
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
static int
pygi_register_api (PyObject *d)
{
    PyObject *api;

    pygobject_api_functions.enum_type = PyGEnum_Type;
    pygobject_api_functions.flags_type = PyGFlags_Type;
    api = PyCapsule_New (&pygobject_api_functions, "gobject._PyGObject_API",
                         NULL);
    if (api == NULL) return -1;
    PyDict_SetItemString (d, "_PyGObject_API", api);
    Py_DECREF (api);
    return 0;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
static int
pygi_register_constants (PyObject *m)
{
    /* PyFloat_ return a new ref, and add object takes the ref */
    PyModule_AddObject (m, "G_MINFLOAT", pygi_gfloat_to_py (G_MINFLOAT));
    PyModule_AddObject (m, "G_MAXFLOAT", pygi_gfloat_to_py (G_MAXFLOAT));
    PyModule_AddObject (m, "G_MINDOUBLE", pygi_gdouble_to_py (G_MINDOUBLE));
    PyModule_AddObject (m, "G_MAXDOUBLE", pygi_gdouble_to_py (G_MAXDOUBLE));
    PyModule_AddIntConstant (m, "G_MINSHORT", G_MINSHORT);
    PyModule_AddIntConstant (m, "G_MAXSHORT", G_MAXSHORT);
    PyModule_AddIntConstant (m, "G_MAXUSHORT", G_MAXUSHORT);
    PyModule_AddIntConstant (m, "G_MININT", G_MININT);
    PyModule_AddIntConstant (m, "G_MAXINT", G_MAXINT);
    PyModule_AddObject (m, "G_MAXUINT", pygi_guint_to_py (G_MAXUINT));
    PyModule_AddObject (m, "G_MINLONG", pygi_glong_to_py (G_MINLONG));
    PyModule_AddObject (m, "G_MAXLONG", pygi_glong_to_py (G_MAXLONG));
    PyModule_AddObject (m, "G_MAXULONG", pygi_gulong_to_py (G_MAXULONG));
    PyModule_AddObject (m, "G_MAXSIZE", pygi_gsize_to_py (G_MAXSIZE));
    PyModule_AddObject (m, "G_MAXSSIZE", pygi_gssize_to_py (G_MAXSSIZE));
    PyModule_AddObject (m, "G_MINSSIZE", pygi_gssize_to_py (G_MINSSIZE));
    PyModule_AddObject (m, "G_MINOFFSET", pygi_gint64_to_py (G_MINOFFSET));
    PyModule_AddObject (m, "G_MAXOFFSET", pygi_gint64_to_py (G_MAXOFFSET));

    PyModule_AddIntConstant (m, "SIGNAL_RUN_FIRST", G_SIGNAL_RUN_FIRST);
    PyModule_AddIntConstant (m, "PARAM_READWRITE", G_PARAM_READWRITE);

    /* The rest of the types are set in __init__.py */
    PyModule_AddObject (m, "TYPE_INVALID",
                        pyg_type_wrapper_new (G_TYPE_INVALID));
    PyModule_AddObject (m, "TYPE_GSTRING",
                        pyg_type_wrapper_new (G_TYPE_GSTRING));

    return 0;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
static int
pygi_register_version_tuples (PyObject *d)
{
    PyObject *tuple;

    /* pygobject version */
    tuple = Py_BuildValue ("(iii)", PYGOBJECT_MAJOR_VERSION,
                           PYGOBJECT_MINOR_VERSION, PYGOBJECT_MICRO_VERSION);
    PyDict_SetItemString (d, "pygobject_version", tuple);
    Py_DECREF (tuple);
    return 0;
}

static PyModuleDef_Slot _gi_slots[] = {
    { Py_mod_exec, _gi_exec },
    { 0, NULL },
};

static struct PyModuleDef __gimodule = {
    PyModuleDef_HEAD_INIT,
    "_gi",
    NULL,
    0,
    _gi_functions,
    _gi_slots,
    NULL,
    NULL,
    NULL,
};

#ifdef __GNUC__
#define PYGI_MODINIT_FUNC                                                     \
    __attribute__ ((visibility ("default"))) PyMODINIT_FUNC
#else
#define PYGI_MODINIT_FUNC PyMODINIT_FUNC
#endif

PYGI_MODINIT_FUNC PyInit__gi (void);

PYGI_MODINIT_FUNC
PyInit__gi (void) { return PyModuleDef_Init (&__gimodule); }

static int
_gi_exec (PyObject *module)
{
    PyObject *api;
    PyObject *module_dict = PyModule_GetDict (module);
    int ret;

#if PY_VERSION_HEX < 0x03090000 || defined(PYPY_VERSION)
    /* Deprecated since 3.9 */
    /* Except in PyPy it's still not a no-op: https://foss.heptapod.net/pypy/pypy/-/issues/3691, https://github.com/pypy/pypy/issues/3690 */

    /* Always enable Python threads since we cannot predict which GI repositories
     * might accept Python callbacks run within non-Python threads or might trigger
     * toggle ref notifications.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=709223
     */
    PyEval_InitThreads ();
#endif

    PyModule_AddStringConstant (module, "__package__", "gi._gi");

    if ((ret = pygi_foreign_init ()) < 0) return ret;
    if ((ret = pygi_error_register_types (module)) < 0) return ret;
    if ((ret = pygi_repository_register_types (module)) < 0) return ret;
    if ((ret = pygi_info_register_types (module)) < 0) return ret;
    if ((ret = pygi_type_register_types (module_dict)) < 0) return ret;
    if ((ret = pygi_pointer_register_types (module_dict)) < 0) return ret;
    if ((ret = pygi_struct_register_types (module)) < 0) return ret;
    if ((ret = pygi_gboxed_register_types (module_dict)) < 0) return ret;
    if ((ret = pygi_fundamental_register_types (module)) < 0) return ret;
    if ((ret = pygi_boxed_register_types (module)) < 0) return ret;
    if ((ret = pygi_ccallback_register_types (module)) < 0) return ret;
    if ((ret = pygi_resulttuple_register_types (module)) < 0) return ret;
    if ((ret = pygi_async_register_types (module) < 0)) return ret;

    if ((ret = pygi_spawn_register_types (module_dict)) < 0) return ret;
    if ((ret = pygi_option_context_register_types (module_dict)) < 0)
        return ret;
    if ((ret = pygi_option_group_register_types (module_dict)) < 0) return ret;

    if ((ret = pygi_register_constants (module)) < 0) return ret;
    if ((ret = pygi_register_version_tuples (module_dict)) < 0) return ret;
    if ((ret = pygi_register_warnings (module_dict)) < 0) return ret;
    if ((ret = pyi_object_register_types (module_dict)) < 0) return ret;
    if ((ret = pygi_interface_register_types (module_dict)) < 0) return ret;
    if ((ret = pygi_enum_register_types (module)) < 0) return ret;
    if ((ret = pygi_flags_register_types (module)) < 0) return ret;
    if ((ret = pygi_register_api (module_dict)) < 0) return ret;

    PyGIWarning = PyErr_NewException ("gi.PyGIWarning", PyExc_Warning, NULL);
    if (PyGIWarning == NULL) return -1;

    PyGIDeprecationWarning = PyErr_NewException (
        "gi.PyGIDeprecationWarning", PyExc_DeprecationWarning, NULL);

    /* Place holder object used to fill in "from Python" argument lists
     * for values not supplied by the caller but support a GI default.
     */
    _PyGIDefaultArgPlaceholder = PyList_New (0);

    Py_INCREF (PyGIWarning);
    PyModule_AddObject (module, "PyGIWarning", PyGIWarning);

    Py_INCREF (PyGIDeprecationWarning);
    PyModule_AddObject (module, "PyGIDeprecationWarning",
                        PyGIDeprecationWarning);

    api = PyCapsule_New ((void *)&CAPI, "gi._API", NULL);
    if (api == NULL) {
        return -1;
    }
    PyModule_AddObject (module, "_API", api);

    return 0;
}
