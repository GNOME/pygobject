/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   gobjectmodule.c: wrapper for the gobject library.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gobject/gvaluecollector.h>
#include <girepository.h>
#include <pyglib.h>
#include <pythread.h>
#include "pygobject-private.h"
#include "pygboxed.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pyginterface.h"
#include "pygparamspec.h"
#include "pygpointer.h"
#include "pygtype.h"
#include "pygoptiongroup.h"

#include "pygi-value.h"
#include "pygi-error.h"
#include "pygi-property.h"

static GHashTable *log_handlers = NULL;
static gboolean log_handlers_disabled = FALSE;

static void pyg_flags_add_constants(PyObject *module, GType flags_type,
				    const gchar *strip_prefix);


/* -------------- GDK threading hooks ---------------------------- */

/**
 * pyg_set_thread_block_funcs:
 * Deprecated, only available for ABI compatibility.
 */
static void
_pyg_set_thread_block_funcs (PyGThreadBlockFunc block_threads_func,
			     PyGThreadBlockFunc unblock_threads_func)
{
    PyGILState_STATE state = pyglib_gil_state_ensure ();
    PyErr_Warn (PyExc_DeprecationWarning,
                "Using pyg_set_thread_block_funcs is not longer needed. "
                "PyGObject always uses Py_BLOCK/UNBLOCK_THREADS.");
    pyglib_gil_state_release (state);
}

/**
 * pyg_destroy_notify:
 * @user_data: a PyObject pointer.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call Py_DECREF on the data.
 */
void
pyg_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();
    Py_DECREF(obj);
    pyglib_gil_state_release(state);
}


/* ---------------- gobject module functions -------------------- */

static PyObject *
pyg_type_name (PyObject *self, PyObject *args)
{
    PyObject *gtype;
    GType type;
    const gchar *name;

#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_name is deprecated; "
		   "use GType.name instead"))
        return NULL;
#endif

    if (!PyArg_ParseTuple(args, "O:gobject.type_name", &gtype))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    name = g_type_name(type);
    if (name)
	return PYGLIB_PyUnicode_FromString(name);
    PyErr_SetString(PyExc_RuntimeError, "unknown typecode");
    return NULL;
}

static PyObject *
pyg_type_from_name (PyObject *self, PyObject *args)
{
    const gchar *name;
    GType type;
    PyObject *repr = NULL;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_from_name is deprecated; "
		   "use GType.from_name instead"))
        return NULL;
#endif
    if (!PyArg_ParseTuple(args, "s:gobject.type_from_name", &name))
	return NULL;
    type = g_type_from_name(name);
    if (type != 0)
	return pyg_type_wrapper_new(type);
    repr = PyObject_Repr((PyObject*)self);
    PyErr_Format(PyExc_RuntimeError, "%s: unknown type name: %s",
         PYGLIB_PyUnicode_AsString(repr),
		 name);
    Py_DECREF(repr);
    return NULL;
}

static PyObject *
pyg_type_is_a (PyObject *self, PyObject *args)
{
    PyObject *gtype, *gparent;
    GType type, parent;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_is_a is deprecated; "
		   "use GType.is_a instead"))
        return NULL;
#endif
    if (!PyArg_ParseTuple(args, "OO:gobject.type_is_a", &gtype, &gparent))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    if ((parent = pyg_type_from_object(gparent)) == 0)
	return NULL;
    return PyBool_FromLong(g_type_is_a(type, parent));
}

static void
pyg_object_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec, *py_value;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();

    object_wrapper = pygobject_new(object);

    if (object_wrapper == NULL) {
	pyglib_gil_state_release(state);
	return;
    }

    py_pspec = pyg_param_spec_new(pspec);
    py_value = pyg_value_as_pyobject (value, TRUE);

    retval = PyObject_CallMethod(object_wrapper, "do_set_property",
				 "OO", py_pspec, py_value);
    if (retval) {
	Py_DECREF(retval);
    } else {
	PyErr_Print();
    }

    Py_DECREF(object_wrapper);
    Py_DECREF(py_pspec);
    Py_DECREF(py_value);

    pyglib_gil_state_release(state);
}

static void
pyg_object_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();

    object_wrapper = pygobject_new(object);

    if (object_wrapper == NULL) {
	pyglib_gil_state_release(state);
	return;
    }

    retval = pygi_call_do_get_property (object_wrapper, pspec);
    if (retval && pyg_value_from_pyobject (value, retval) < 0) {
        PyErr_Print();
    }
    Py_DECREF(object_wrapper);
    Py_XDECREF(retval);

    pyglib_gil_state_release(state);
}

typedef struct _PyGSignalAccumulatorData {
    PyObject *callable;
    PyObject *user_data;
} PyGSignalAccumulatorData;

static gboolean
_pyg_signal_accumulator(GSignalInvocationHint *ihint,
                        GValue *return_accu,
                        const GValue *handler_return,
                        gpointer _data)
{
    PyObject *py_ihint, *py_return_accu, *py_handler_return, *py_detail;
    PyObject *py_retval;
    gboolean retval = FALSE;
    PyGSignalAccumulatorData *data = _data;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();
    if (ihint->detail)
        py_detail = PYGLIB_PyUnicode_FromString(g_quark_to_string(ihint->detail));
    else {
        Py_INCREF(Py_None);
        py_detail = Py_None;
    }

    py_ihint = Py_BuildValue("lNi", (long int) ihint->signal_id,
                             py_detail, ihint->run_type);
    py_handler_return = pyg_value_as_pyobject(handler_return, TRUE);
    py_return_accu = pyg_value_as_pyobject(return_accu, FALSE);
    if (data->user_data)
        py_retval = PyObject_CallFunction(data->callable, "NNNO", py_ihint,
                                          py_return_accu, py_handler_return,
                                          data->user_data);
    else
        py_retval = PyObject_CallFunction(data->callable, "NNN", py_ihint,
                                          py_return_accu, py_handler_return);
    if (!py_retval)
	PyErr_Print();
    else {
        if (!PyTuple_Check(py_retval) || PyTuple_Size(py_retval) != 2) {
            PyErr_SetString(PyExc_TypeError, "accumulator function must return"
                            " a (bool, object) tuple");
            PyErr_Print();
        } else {
            retval = PyObject_IsTrue(PyTuple_GET_ITEM(py_retval, 0));
            if (pyg_value_from_pyobject(return_accu, PyTuple_GET_ITEM(py_retval, 1))) {
                PyErr_Print();
            }
        }
        Py_DECREF(py_retval);
    }
    pyglib_gil_state_release(state);
    return retval;
}

static gboolean
create_signal (GType instance_type, const gchar *signal_name, PyObject *tuple)
{
    GSignalFlags signal_flags;
    PyObject *py_return_type, *py_param_types;
    GType return_type;
    guint n_params, i;
    GType *param_types;
    guint signal_id;
    GSignalAccumulator accumulator = NULL;
    PyGSignalAccumulatorData *accum_data = NULL;
    PyObject *py_accum = NULL, *py_accum_data = NULL;

    if (!PyArg_ParseTuple(tuple, "iOO|OO", &signal_flags, &py_return_type,
			  &py_param_types, &py_accum, &py_accum_data))
    {
	gchar buf[128];

	PyErr_Clear();
	g_snprintf(buf, sizeof(buf),
		   "value for __gsignals__['%s'] not in correct format", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }

    if (py_accum && py_accum != Py_None && !PyCallable_Check(py_accum))
    {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "accumulator for __gsignals__['%s'] must be callable", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }

    return_type = pyg_type_from_object(py_return_type);
    if (!return_type)
	return FALSE;
    if (!PySequence_Check(py_param_types)) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "third element of __gsignals__['%s'] tuple must be a sequence", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }
    n_params = PySequence_Length(py_param_types);
    param_types = g_new(GType, n_params);
    for (i = 0; i < n_params; i++) {
	PyObject *item = PySequence_GetItem(py_param_types, i);

	param_types[i] = pyg_type_from_object(item);
	if (param_types[i] == 0) {
	    Py_DECREF(item);
	    g_free(param_types);
	    return FALSE;
	}
	Py_DECREF(item);
    }

    if (py_accum != NULL && py_accum != Py_None) {
        accum_data = g_new(PyGSignalAccumulatorData, 1);
        accum_data->callable = py_accum;
        Py_INCREF(py_accum);
        accum_data->user_data = py_accum_data;
        Py_XINCREF(py_accum_data);
        accumulator = _pyg_signal_accumulator;
    }

    signal_id = g_signal_newv(signal_name, instance_type, signal_flags,
			      pyg_signal_class_closure_get(),
			      accumulator, accum_data,
			      gi_cclosure_marshal_generic,
			      return_type, n_params, param_types);
    g_free(param_types);

    if (signal_id == 0) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf), "could not create signal for %s",
		   signal_name);
	PyErr_SetString(PyExc_RuntimeError, buf);
	return FALSE;
    }
    return TRUE;
}

static gboolean
override_signal(GType instance_type, const gchar *signal_name)
{
    guint signal_id;

    signal_id = g_signal_lookup(signal_name, instance_type);
    if (!signal_id) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf), "could not look up %s", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }
    g_signal_override_class_closure(signal_id, instance_type,
				    pyg_signal_class_closure_get());
    return TRUE;
}

static PyObject *
add_signals (GObjectClass *klass, PyObject *signals)
{
    gboolean ret = TRUE;
    Py_ssize_t pos = 0;
    PyObject *key, *value, *overridden_signals = NULL;
    GType instance_type = G_OBJECT_CLASS_TYPE (klass);

    overridden_signals = PyDict_New();
    while (PyDict_Next(signals, &pos, &key, &value)) {
	const gchar *signal_name;
        gchar *signal_name_canon, *c;

	if (!PYGLIB_PyUnicode_Check(key)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gsignals__ keys must be strings");
	    ret = FALSE;
	    break;
	}
	signal_name = PYGLIB_PyUnicode_AsString (key);

	if (value == Py_None ||
	    (PYGLIB_PyUnicode_Check(value) &&
	     !strcmp(PYGLIB_PyUnicode_AsString(value), "override")))
        {
              /* canonicalize signal name, replacing '-' with '_' */
            signal_name_canon = g_strdup(signal_name);
            for (c = signal_name_canon; *c; ++c)
                if (*c == '-')
                    *c = '_';
            if (PyDict_SetItemString(overridden_signals,
				     signal_name_canon, key)) {
                g_free(signal_name_canon);
                ret = FALSE;
                break;
            }
            g_free(signal_name_canon);

	    ret = override_signal(instance_type, signal_name);
	} else {
	    ret = create_signal(instance_type, signal_name, value);
	}

	if (!ret)
	    break;
    }
    if (ret)
        return overridden_signals;
    else {
        Py_XDECREF(overridden_signals);
        return NULL;
    }
}

static GParamSpec *
create_property (const gchar  *prop_name,
		 GType         prop_type,
		 const gchar  *nick,
		 const gchar  *blurb,
		 PyObject     *args,
		 GParamFlags   flags)
{
    GParamSpec *pspec = NULL;

    switch (G_TYPE_FUNDAMENTAL(prop_type)) {
    case G_TYPE_CHAR:
	{
	    gchar minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ccc", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_char (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_UCHAR:
	{
	    gchar minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ccc", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_uchar (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_BOOLEAN:
	{
	    gboolean default_value;

	    if (!PyArg_ParseTuple(args, "i", &default_value))
		return NULL;
	    pspec = g_param_spec_boolean (prop_name, nick, blurb,
					  default_value, flags);
	}
	break;
    case G_TYPE_INT:
	{
	    gint minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "iii", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_int (prop_name, nick, blurb, minimum,
				      maximum, default_value, flags);
	}
	break;
    case G_TYPE_UINT:
	{
	    guint minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "III", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_uint (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_LONG:
	{
	    glong minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "lll", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_long (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_ULONG:
	{
	    gulong minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "kkk", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_ulong (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_INT64:
	{
	    gint64 minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "LLL", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_int64 (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_UINT64:
	{
	    guint64 minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "KKK", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_uint64 (prop_name, nick, blurb, minimum,
					 maximum, default_value, flags);
	}
	break;
    case G_TYPE_ENUM:
	{
	    gint default_value;
	    PyObject *pydefault;

	    if (!PyArg_ParseTuple(args, "O", &pydefault))
		return NULL;

	    if (pyg_enum_get_value(prop_type, pydefault,
				   (gint *)&default_value))
		return NULL;

	    pspec = g_param_spec_enum (prop_name, nick, blurb,
				       prop_type, default_value, flags);
	}
	break;
    case G_TYPE_FLAGS:
	{
	    guint default_value;
	    PyObject *pydefault;

	    if (!PyArg_ParseTuple(args, "O", &pydefault))
		return NULL;

	    if (pyg_flags_get_value(prop_type, pydefault,
				    &default_value))
		return NULL;

	    pspec = g_param_spec_flags (prop_name, nick, blurb,
					prop_type, default_value, flags);
	}
	break;
    case G_TYPE_FLOAT:
	{
	    gfloat minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "fff", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_float (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_DOUBLE:
	{
	    gdouble minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ddd", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_double (prop_name, nick, blurb, minimum,
					 maximum, default_value, flags);
	}
	break;
    case G_TYPE_STRING:
	{
	    const gchar *default_value;

	    if (!PyArg_ParseTuple(args, "z", &default_value))
		return NULL;
	    pspec = g_param_spec_string (prop_name, nick, blurb,
					 default_value, flags);
	}
	break;
    case G_TYPE_PARAM:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_param (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_BOXED:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_boxed (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_POINTER:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	if (prop_type == G_TYPE_GTYPE)
	    pspec = g_param_spec_gtype (prop_name, nick, blurb, G_TYPE_NONE, flags);
	else
	    pspec = g_param_spec_pointer (prop_name, nick, blurb, flags);
	break;
    case G_TYPE_OBJECT:
    case G_TYPE_INTERFACE:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_object (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_VARIANT:
	{
	    PyObject *pydefault;
            GVariant *default_value = NULL;

	    if (!PyArg_ParseTuple(args, "O", &pydefault))
		return NULL;
            if (pydefault != Py_None)
                default_value = pyg_boxed_get (pydefault, GVariant);
	    pspec = g_param_spec_variant (prop_name, nick, blurb, G_VARIANT_TYPE_ANY, default_value, flags);
	}
	break;
    default:
	/* unhandled pspec type ... */
	break;
    }

    if (!pspec) {
	char buf[128];

	g_snprintf(buf, sizeof(buf), "could not create param spec for type %s",
		   g_type_name(prop_type));
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
    }

    return pspec;
}

static GParamSpec *
pyg_param_spec_from_object (PyObject *tuple)
{
    gint val_length;
    const gchar *prop_name;
    GType prop_type;
    const gchar *nick, *blurb;
    PyObject *slice, *item, *py_prop_type;
    GParamSpec *pspec;

    val_length = PyTuple_Size(tuple);
    if (val_length < 4) {
	PyErr_SetString(PyExc_TypeError,
			"paramspec tuples must be at least 4 elements long");
	return NULL;
    }

    slice = PySequence_GetSlice(tuple, 0, 4);
    if (!slice) {
	return NULL;
    }

    if (!PyArg_ParseTuple(slice, "sOzz", &prop_name, &py_prop_type, &nick, &blurb)) {
	Py_DECREF(slice);
	return NULL;
    }

    Py_DECREF(slice);

    prop_type = pyg_type_from_object(py_prop_type);
    if (!prop_type) {
	return NULL;
    }

    item = PyTuple_GetItem(tuple, val_length-1);
    if (!PYGLIB_PyLong_Check(item)) {
	PyErr_SetString(PyExc_TypeError,
			"last element in tuple must be an int");
	return NULL;
    }

    /* slice is the extra items in the tuple */
    slice = PySequence_GetSlice(tuple, 4, val_length-1);
    pspec = create_property(prop_name, prop_type,
			    nick, blurb, slice,
			    PYGLIB_PyLong_AsLong(item));

    return pspec;
}

static gboolean
add_properties (GObjectClass *klass, PyObject *properties)
{
    gboolean ret = TRUE;
    Py_ssize_t pos = 0;
    PyObject *key, *value;

    while (PyDict_Next(properties, &pos, &key, &value)) {
	const gchar *prop_name;
	GType prop_type;
	const gchar *nick, *blurb;
	GParamFlags flags;
	gint val_length;
	PyObject *slice, *item, *py_prop_type;
	GParamSpec *pspec;

	/* values are of format (type,nick,blurb, type_specific_args, flags) */

	if (!PYGLIB_PyUnicode_Check(key)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ keys must be strings");
	    ret = FALSE;
	    break;
	}
	prop_name = PYGLIB_PyUnicode_AsString (key);

	if (!PyTuple_Check(value)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ values must be tuples");
	    ret = FALSE;
	    break;
	}
	val_length = PyTuple_Size(value);
	if (val_length < 4) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ values must be at least 4 elements long");
	    ret = FALSE;
	    break;
	}

	slice = PySequence_GetSlice(value, 0, 3);
	if (!slice) {
	    ret = FALSE;
	    break;
	}
	if (!PyArg_ParseTuple(slice, "Ozz", &py_prop_type, &nick, &blurb)) {
	    Py_DECREF(slice);
	    ret = FALSE;
	    break;
	}
	Py_DECREF(slice);
	prop_type = pyg_type_from_object(py_prop_type);
	if (!prop_type) {
	    ret = FALSE;
	    break;
	}
	item = PyTuple_GetItem(value, val_length-1);
	if (!PYGLIB_PyLong_Check(item)) {
	    PyErr_SetString(PyExc_TypeError,
		"last element in __gproperties__ value tuple must be an int");
	    ret = FALSE;
	    break;
	}
	flags = PYGLIB_PyLong_AsLong(item);

	/* slice is the extra items in the tuple */
	slice = PySequence_GetSlice(value, 3, val_length-1);
	pspec = create_property(prop_name, prop_type, nick, blurb,
				slice, flags);
	Py_DECREF(slice);

	if (pspec) {
	    g_object_class_install_property(klass, 1, pspec);
	} else {
            PyObject *type, *value, *traceback;
	    ret = FALSE;
            PyErr_Fetch(&type, &value, &traceback);
            if (PYGLIB_PyUnicode_Check(value)) {
                char msg[256];
                g_snprintf(msg, 256,
			   "%s (while registering property '%s' for GType '%s')",
               PYGLIB_PyUnicode_AsString(value),
			   prop_name, G_OBJECT_CLASS_NAME(klass));
                Py_DECREF(value);
                value = PYGLIB_PyUnicode_FromString(msg);
            }
            PyErr_Restore(type, value, traceback);
	    break;
	}
    }

    return ret;
}

static void
pyg_object_class_init(GObjectClass *class, PyObject *py_class)
{
    PyObject *gproperties, *gsignals, *overridden_signals;
    PyObject *class_dict = ((PyTypeObject*) py_class)->tp_dict;

    class->set_property = pyg_object_set_property;
    class->get_property = pyg_object_get_property;

    /* install signals */
    /* we look this up in the instance dictionary, so we don't
     * accidentally get a parent type's __gsignals__ attribute. */
    gsignals = PyDict_GetItemString(class_dict, "__gsignals__");
    if (gsignals) {
	if (!PyDict_Check(gsignals)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gsignals__ attribute not a dict!");
	    return;
	}
	if (!(overridden_signals = add_signals(class, gsignals))) {
	    return;
	}
        if (PyDict_SetItemString(class_dict, "__gsignals__",
				 overridden_signals)) {
            return;
        }
        Py_DECREF(overridden_signals);

        PyDict_DelItemString(class_dict, "__gsignals__");
    } else {
	PyErr_Clear();
    }

    /* install properties */
    /* we look this up in the instance dictionary, so we don't
     * accidentally get a parent type's __gproperties__ attribute. */
    gproperties = PyDict_GetItemString(class_dict, "__gproperties__");
    if (gproperties) {
	if (!PyDict_Check(gproperties)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ attribute not a dict!");
	    return;
	}
	if (!add_properties(class, gproperties)) {
	    return;
	}
	PyDict_DelItemString(class_dict, "__gproperties__");
	/* Borrowed reference. Py_DECREF(gproperties); */
    } else {
	PyErr_Clear();
    }
}

static void
pyg_register_class_init(GType gtype, PyGClassInitFunc class_init)
{
    GSList *list;

    list = g_type_get_qdata(gtype, pygobject_class_init_key);
    list = g_slist_prepend(list, class_init);
    g_type_set_qdata(gtype, pygobject_class_init_key, list);
}

static int
pyg_run_class_init(GType gtype, gpointer gclass, PyTypeObject *pyclass)
{
    GSList *list;
    PyGClassInitFunc class_init;
    GType parent_type;
    int rv;

    parent_type = g_type_parent(gtype);
    if (parent_type) {
        rv = pyg_run_class_init(parent_type, gclass, pyclass);
        if (rv)
	    return rv;
    }

    list = g_type_get_qdata(gtype, pygobject_class_init_key);
    for (; list; list = list->next) {
	class_init = list->data;
        rv = class_init(gclass, pyclass);
        if (rv)
	    return rv;
    }

    return 0;
}

static PyObject *
_wrap_pyg_type_register(PyObject *self, PyObject *args)
{
    PyTypeObject *class;
    char *type_name = NULL;

    if (!PyArg_ParseTuple(args, "O!|z:gobject.type_register",
			  &PyType_Type, &class, &type_name))
	return NULL;
    if (!PyType_IsSubtype(class, &PyGObject_Type)) {
	PyErr_SetString(PyExc_TypeError,
			"argument must be a GObject subclass");
	return NULL;
    }

      /* Check if type already registered */
    if (pyg_type_from_object((PyObject *) class) ==
        pyg_type_from_object((PyObject *) class->tp_base))
    {
        if (pyg_type_register(class, type_name))
            return NULL;
    }

    Py_INCREF(class);
    return (PyObject *) class;
}

static char *
get_type_name_for_class(PyTypeObject *class)
{
    gint i, name_serial;
    char name_serial_str[16];
    PyObject *module;
    char *type_name = NULL;

    /* make name for new GType */
    name_serial = 1;
    /* give up after 1000 tries, just in case.. */
    while (name_serial < 1000)
    {
	g_free(type_name);
	g_snprintf(name_serial_str, 16, "-v%i", name_serial);
	module = PyObject_GetAttrString((PyObject *)class, "__module__");
	if (module && PYGLIB_PyUnicode_Check(module)) {
	    type_name = g_strconcat(PYGLIB_PyUnicode_AsString(module), ".",
				    class->tp_name,
				    name_serial > 1 ? name_serial_str : NULL,
				    NULL);
	    Py_DECREF(module);
	} else {
	    if (module)
		Py_DECREF(module);
	    else
		PyErr_Clear();
	    type_name = g_strconcat(class->tp_name,
				    name_serial > 1 ? name_serial_str : NULL,
				    NULL);
	}
	/* convert '.' in type name to '+', which isn't banned (grumble) */
	for (i = 0; type_name[i] != '\0'; i++)
	    if (type_name[i] == '.')
		type_name[i] = '+';
	if (g_type_from_name(type_name) == 0)
	    break;              /* we now have a unique name */
	++name_serial;
    }

    return type_name;
}


static GPrivate pygobject_construction_wrapper;

static inline void
pygobject_init_wrapper_set(PyObject *wrapper)
{
    g_private_set(&pygobject_construction_wrapper, wrapper);
}

static inline PyObject *
pygobject_init_wrapper_get(void)
{
    return (PyObject *) g_private_get(&pygobject_construction_wrapper);
}

int
pygobject_constructv(PyGObject  *self,
                     guint       n_parameters,
                     GParameter *parameters)
{
    GObject *obj;

    g_assert (self->obj == NULL);
    pygobject_init_wrapper_set((PyObject *) self);
    obj = g_object_newv(pyg_type_from_object((PyObject *) self),
                        n_parameters, parameters);

    if (g_object_is_floating (obj))
        self->private_flags.flags |= PYGOBJECT_GOBJECT_WAS_FLOATING;
    pygobject_sink (obj);

    pygobject_init_wrapper_set(NULL);
    self->obj = obj;
    pygobject_register_wrapper((PyObject *) self);

    return 0;
}

static void
pygobject__g_instance_init(GTypeInstance   *instance,
                           gpointer         g_class)
{
    GObject *object = (GObject *) instance;
    PyObject *wrapper, *args, *kwargs;

    wrapper = g_object_get_qdata(object, pygobject_wrapper_key);
    if (wrapper == NULL) {
        wrapper = pygobject_init_wrapper_get();
        if (wrapper && ((PyGObject *) wrapper)->obj == NULL) {
            ((PyGObject *) wrapper)->obj = object;
            pygobject_register_wrapper(wrapper);
        }
    }
    pygobject_init_wrapper_set(NULL);
    if (wrapper == NULL) {
          /* this looks like a python object created through
           * g_object_new -> we have no python wrapper, so create it
           * now */
        PyGILState_STATE state;
        state = pyglib_gil_state_ensure();
        wrapper = pygobject_new_full(object,
                                     /*steal=*/ FALSE,
                                     g_class);

        /* float the wrapper ref here because we are going to orphan it
         * so we don't destroy the wrapper. The next call to pygobject_new_full
         * will take the ref */
        pygobject_ref_float ((PyGObject *) wrapper);
        args = PyTuple_New(0);
        kwargs = PyDict_New();
        if (Py_TYPE(wrapper)->tp_init(wrapper, args, kwargs))
            PyErr_Print();

        Py_DECREF(args);
        Py_DECREF(kwargs);
        pyglib_gil_state_release(state);
    }
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
pyg_type_add_interfaces(PyTypeObject *class, GType instance_type,
                        PyObject *bases,
                        GType *parent_interfaces, guint n_parent_interfaces)
{
    int i;

    if (!bases) {
        g_warning("type has no bases");
        return;
    }

    for (i = 0; i < PyTuple_GET_SIZE(bases); ++i) {
        PyObject *base = PyTuple_GET_ITEM(bases, i);
        GType itype;
        const GInterfaceInfo *iinfo;
        GInterfaceInfo iinfo_copy;

        /* 'base' can also be a PyClassObject, see bug #566571. */
        if (!PyType_Check(base))
            continue;

        if (!PyType_IsSubtype((PyTypeObject*) base, &PyGInterface_Type))
            continue;

        itype = pyg_type_from_object(base);

        /* Happens for _implementations_ of an interface. */
        if (!G_TYPE_IS_INTERFACE(itype))
            continue;

        iinfo = pyg_lookup_interface_info(itype);
        if (!iinfo) {
            gchar *error;
            error = g_strdup_printf("Interface type %s "
                                    "has no Python implementation support",
                                    ((PyTypeObject *) base)->tp_name);
            PyErr_Warn(PyExc_RuntimeWarning, error);
            g_free(error);
            continue;
        }

        iinfo_copy = *iinfo;
        iinfo_copy.interface_data = class;
        g_type_add_interface_static(instance_type, itype, &iinfo_copy);
    }
}

int
pyg_type_register(PyTypeObject *class, const char *type_name)
{
    PyObject *gtype;
    GType parent_type, instance_type;
    GType *parent_interfaces;
    guint n_parent_interfaces;
    GTypeQuery query;
    gpointer gclass;
    GTypeInfo type_info = {
	0,    /* class_size */

	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,

	(GClassInitFunc) pyg_object_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class_data */

	0,    /* instance_size */
	0,    /* n_preallocs */
	(GInstanceInitFunc) pygobject__g_instance_init
    };
    gchar *new_type_name;

    /* find the GType of the parent */
    parent_type = pyg_type_from_object((PyObject *)class);
    if (!parent_type)
	return -1;

    parent_interfaces = g_type_interfaces(parent_type, &n_parent_interfaces);

    if (type_name)
	/* care is taken below not to free this */
        new_type_name = (gchar *) type_name;
    else
	new_type_name = get_type_name_for_class(class);

    /* set class_data that will be passed to the class_init function. */
    type_info.class_data = class;

    /* fill in missing values of GTypeInfo struct */
    g_type_query(parent_type, &query);
    type_info.class_size = query.class_size;
    type_info.instance_size = query.instance_size;

    /* create new typecode */
    instance_type = g_type_register_static(parent_type, new_type_name,
					   &type_info, 0);
    if (instance_type == 0) {
	PyErr_Format(PyExc_RuntimeError,
		     "could not create new GType: %s (subclass of %s)",
		     new_type_name,
		     g_type_name(parent_type));

        if (type_name == NULL)
            g_free(new_type_name);

	return -1;
    }

    if (type_name == NULL)
        g_free(new_type_name);

    /* store pointer to the class with the GType */
    Py_INCREF(class);
    g_type_set_qdata(instance_type, g_quark_from_string("PyGObject::class"),
		     class);

    /* Mark this GType as a custom python type */
    g_type_set_qdata(instance_type, pygobject_custom_key,
                     GINT_TO_POINTER (1));

    /* set new value of __gtype__ on class */
    gtype = pyg_type_wrapper_new(instance_type);
    PyObject_SetAttrString((PyObject *)class, "__gtype__", gtype);
    Py_DECREF(gtype);

    /* if no __doc__, set it to the auto doc descriptor */
    if (PyDict_GetItemString(class->tp_dict, "__doc__") == NULL) {
	PyDict_SetItemString(class->tp_dict, "__doc__",
			     pyg_object_descr_doc_get());
    }

    /*
     * Note, all interfaces need to be registered before the first
     * g_type_class_ref(), see bug #686149.
     *
     * See also comment above pyg_type_add_interfaces().
     */
    pyg_type_add_interfaces(class, instance_type, class->tp_bases,
                            parent_interfaces, n_parent_interfaces);


    gclass = g_type_class_ref(instance_type);
    if (PyErr_Occurred() != NULL) {
        g_type_class_unref(gclass);
        g_free(parent_interfaces);
        return -1;
    }

    if (pyg_run_class_init(instance_type, gclass, class)) {
        g_type_class_unref(gclass);
        g_free(parent_interfaces);
        return -1;
    }
    g_type_class_unref(gclass);
    g_free(parent_interfaces);

    if (PyErr_Occurred() != NULL)
        return -1;
    return 0;
}

static PyObject *
pyg_signal_new(PyObject *self, PyObject *args)
{
    gchar *signal_name;
    PyObject *py_type;
    GSignalFlags signal_flags;
    GType return_type;
    PyObject *py_return_type, *py_param_types;

    GType instance_type = 0;
    Py_ssize_t n_params, i;
    GType *param_types;

    guint signal_id;

    if (!PyArg_ParseTuple(args, "sOiOO:gobject.signal_new", &signal_name,
			  &py_type, &signal_flags, &py_return_type,
			  &py_param_types))
	return NULL;

    instance_type = pyg_type_from_object(py_type);
    if (!instance_type)
	return NULL;
    if (!(G_TYPE_IS_INSTANTIATABLE(instance_type) || G_TYPE_IS_INTERFACE(instance_type))) {
	PyErr_SetString(PyExc_TypeError,
			"argument 2 must be an object type or interface type");
	return NULL;
    }

    return_type = pyg_type_from_object(py_return_type);
    if (!return_type)
	return NULL;

    if (!PySequence_Check(py_param_types)) {
	PyErr_SetString(PyExc_TypeError,
			"argument 5 must be a sequence of GType codes");
	return NULL;
    }
    n_params = PySequence_Length(py_param_types);
    param_types = g_new(GType, n_params);
    for (i = 0; i < n_params; i++) {
	PyObject *item = PySequence_GetItem(py_param_types, i);

	param_types[i] = pyg_type_from_object(item);
	if (param_types[i] == 0) {
	    PyErr_Clear();
	    Py_DECREF(item);
	    PyErr_SetString(PyExc_TypeError,
			    "argument 5 must be a sequence of GType codes");
	    g_free(param_types);
	    return NULL;
	}
	Py_DECREF(item);
    }

    signal_id = g_signal_newv(signal_name, instance_type, signal_flags,
			      pyg_signal_class_closure_get(),
			      (GSignalAccumulator)0, NULL,
			      (GSignalCMarshaller)0,
			      return_type, n_params, param_types);
    g_free(param_types);
    if (signal_id != 0)
	return PYGLIB_PyLong_FromLong(signal_id);
    PyErr_SetString(PyExc_RuntimeError, "could not create signal");
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

    if (!PyArg_ParseTuple(args, "O:gobject.list_properties",
			  &py_itype))
	return NULL;
    if ((itype = pyg_type_from_object(py_itype)) == 0)
	return NULL;

    if (G_TYPE_IS_INTERFACE(itype)) {
        iface = g_type_default_interface_ref(itype);
        if (!iface) {
            PyErr_SetString(PyExc_RuntimeError,
                            "could not get a reference to interface type");
            return NULL;
        }
        specs = g_object_interface_list_properties(iface, &nprops);
    } else if (g_type_is_a(itype, G_TYPE_OBJECT)) {
        class = g_type_class_ref(itype);
        if (!class) {
            PyErr_SetString(PyExc_RuntimeError,
                            "could not get a reference to type class");
            return NULL;
        }
        specs = g_object_class_list_properties(class, &nprops);
    } else {
	PyErr_SetString(PyExc_TypeError,
                        "type must be derived from GObject or an interface");
	return NULL;
    }

    list = PyTuple_New(nprops);
    if (list == NULL) {
	g_free(specs);
	g_type_class_unref(class);
	return NULL;
    }
    for (i = 0; i < nprops; i++) {
	PyTuple_SetItem(list, i, pyg_param_spec_new(specs[i]));
    }
    g_free(specs);
    if (class)
        g_type_class_unref(class);
    else
        g_type_default_interface_unref(iface);

    return list;
}

static PyObject *
pyg_object_new (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pytype;
    GType type;
    GObject *obj = NULL;
    GObjectClass *class;
    guint n_params = 0, i;
    GParameter *params = NULL;

    if (!PyArg_ParseTuple (args, "O:gobject.new", &pytype)) {
	return NULL;
    }

    if ((type = pyg_type_from_object (pytype)) == 0)
	return NULL;

    if (G_TYPE_IS_ABSTRACT(type)) {
	PyErr_Format(PyExc_TypeError, "cannot create instance of abstract "
		     "(non-instantiable) type `%s'", g_type_name(type));
	return NULL;
    }

    if ((class = g_type_class_ref (type)) == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"could not get a reference to type class");
	return NULL;
    }

    if (!pygobject_prepare_construct_properties (class, kwargs, &n_params, &params))
        goto cleanup;

    obj = g_object_newv(type, n_params, params);
    if (!obj)
	PyErr_SetString (PyExc_RuntimeError, "could not create object");

 cleanup:
    for (i = 0; i < n_params; i++) {
	g_free((gchar *) params[i].name);
	g_value_unset(&params[i].value);
    }
    g_free(params);
    g_type_class_unref(class);

    if (obj) {
        pygobject_sink (obj);
	self = (PyGObject *) pygobject_new((GObject *)obj);
        g_object_unref(obj);
    } else
        self = NULL;

    return (PyObject *) self;
}

gboolean
pyg_handler_marshal(gpointer user_data)
{
    PyObject *tuple, *ret;
    gboolean res;
    PyGILState_STATE state;

    g_return_val_if_fail(user_data != NULL, FALSE);

    state = pyglib_gil_state_ensure();

    tuple = (PyObject *)user_data;
    ret = PyObject_CallObject(PyTuple_GetItem(tuple, 0),
			      PyTuple_GetItem(tuple, 1));
    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }

    pyglib_gil_state_release(state);

    return res;
}

static int
pygobject_gil_state_ensure (void)
{
    return pyglib_gil_state_ensure ();
}

static void
pygobject_gil_state_release (int flag)
{
    pyglib_gil_state_release(flag);
}

/* Only for backwards compatibility */
static int
pygobject_enable_threads(void)
{
    return 0;
}

static PyObject *
pyg_signal_accumulator_true_handled(PyObject *unused, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
		    "signal_accumulator_true_handled can only"
                    " be used as accumulator argument when registering signals");
    return NULL;
}

static gboolean
marshal_emission_hook(GSignalInvocationHint *ihint,
		      guint n_param_values,
		      const GValue *param_values,
		      gpointer user_data)
{
    PyGILState_STATE state;
    gboolean retval = FALSE;
    PyObject *func, *args;
    PyObject *retobj;
    PyObject *params;
    guint i;

    state = pyglib_gil_state_ensure();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values);

    for (i = 0; i < n_param_values; i++) {
	PyObject *item = pyg_value_as_pyobject(&param_values[i], FALSE);

	/* error condition */
	if (!item) {
	    goto out;
	}
	PyTuple_SetItem(params, i, item);
    }

    args = (PyObject *)user_data;
    func = PyTuple_GetItem(args, 0);
    args = PySequence_Concat(params, PyTuple_GetItem(args, 1));
    Py_DECREF(params);

    /* params passed to function may have extra arguments */

    retobj = PyObject_CallObject(func, args);
    Py_DECREF(args);
    if (retobj == NULL) {
        PyErr_Print();
    }

    retval = (retobj == Py_True ? TRUE : FALSE);
    Py_XDECREF(retobj);
out:
    pyglib_gil_state_release(state);
    return retval;
}

static PyObject *
pyg_add_emission_hook(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *data, *repr;
    gchar *name;
    gulong hook_id;
    guint sigid;
    Py_ssize_t len;
    GQuark detail = 0;
    GType gtype;
    PyObject *pygtype;

    len = PyTuple_Size(args);
    if (len < 3) {
	PyErr_SetString(PyExc_TypeError,
			"gobject.add_emission_hook requires at least 3 arguments");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 3);
    if (!PyArg_ParseTuple(first, "OsO:add_emission_hook",
			  &pygtype, &name, &callback)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);

    if ((gtype = pyg_type_from_object(pygtype)) == 0) {
	return NULL;
    }
    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "third argument must be callable");
	return NULL;
    }

    if (!g_signal_parse_name(name, gtype, &sigid, &detail, TRUE)) {
	repr = PyObject_Repr((PyObject*)self);
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
			PYGLIB_PyUnicode_AsString(repr),
		     name);
	Py_DECREF(repr);
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 3, len);
    if (extra_args == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, extra_args);
    if (data == NULL)
      return NULL;

    hook_id = g_signal_add_emission_hook(sigid, detail,
					 marshal_emission_hook,
					 data,
					 (GDestroyNotify)pyg_destroy_notify);

    return PyLong_FromUnsignedLong(hook_id);
}

static PyObject *
pyg__install_metaclass(PyObject *dummy, PyTypeObject *metaclass)
{
    Py_INCREF(metaclass);
    PyGObject_MetaType = metaclass;
    Py_INCREF(metaclass);

    Py_TYPE(&PyGObject_Type) = metaclass;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pyg__gvalue_get(PyObject *module, PyObject *pygvalue)
{
    if (!pyg_boxed_check (pygvalue, G_TYPE_VALUE)) {
        PyErr_SetString (PyExc_TypeError, "Expected GValue argument.");
        return NULL;
    }

    return pyg_value_as_pyobject (pyg_boxed_get(pygvalue, GValue),
                                  /*copy_boxed=*/ TRUE);
}

static PyObject *
pyg__gvalue_set(PyObject *module, PyObject *args)
{
    PyObject *pygvalue;
    PyObject *pyobject;

    if (!PyArg_ParseTuple (args, "OO:_gobject._gvalue_set",
                           &pygvalue, &pyobject))
        return NULL;

    if (!pyg_boxed_check (pygvalue, G_TYPE_VALUE)) {
        PyErr_SetString (PyExc_TypeError, "Expected GValue argument.");
        return NULL;
    }

    if (pyg_value_from_pyobject_with_error (pyg_boxed_get (pygvalue, GValue),
                                            pyobject) == -1)
        return NULL;

    Py_RETURN_NONE;
}

static PyMethodDef _gobject_functions[] = {
    { "type_name", pyg_type_name, METH_VARARGS },
    { "type_from_name", pyg_type_from_name, METH_VARARGS },
    { "type_is_a", pyg_type_is_a, METH_VARARGS },
    { "type_register", _wrap_pyg_type_register, METH_VARARGS },
    { "signal_new", pyg_signal_new, METH_VARARGS },
    { "list_properties",
      pyg_object_class_list_properties, METH_VARARGS },
    { "new",
      (PyCFunction)pyg_object_new, METH_VARARGS|METH_KEYWORDS },
    { "signal_accumulator_true_handled",
      (PyCFunction)pyg_signal_accumulator_true_handled, METH_VARARGS },
    { "add_emission_hook",
      (PyCFunction)pyg_add_emission_hook, METH_VARARGS },
    { "_install_metaclass",
      (PyCFunction)pyg__install_metaclass, METH_O },
    { "_gvalue_get",
      (PyCFunction)pyg__gvalue_get, METH_O },
    { "_gvalue_set",
      (PyCFunction)pyg__gvalue_set, METH_VARARGS },

    { NULL, NULL, 0 }
};


/* ----------------- Constant extraction ------------------------ */

/**
 * pyg_constant_strip_prefix:
 * @name: the constant name.
 * @strip_prefix: the prefix to strip.
 *
 * Advances the pointer @name by strlen(@strip_prefix) characters.  If
 * the resulting name does not start with a letter or underscore, the
 * @name pointer will be rewound.  This is to ensure that the
 * resulting name is a valid identifier.  Hence the returned string is
 * a pointer into the string @name.
 *
 * Returns: the stripped constant name.
 */
const gchar *
pyg_constant_strip_prefix(const gchar *name, const gchar *strip_prefix)
{
    gint prefix_len;
    guint i;

    prefix_len = strlen(strip_prefix);

    /* Check so name starts with strip_prefix, if it doesn't:
     * return the rest of the part which doesn't match
     */
    for (i = 0; i < prefix_len; i++) {
	if (name[i] != strip_prefix[i] && name[i] != '_') {
	    return &name[i];
	}
    }

    /* strip off prefix from value name, while keeping it a valid
     * identifier */
    for (i = prefix_len; i >= 0; i--) {
	if (g_ascii_isalpha(name[i]) || name[i] == '_') {
	    return &name[i];
	}
    }
    return name;
}

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
pyg_enum_add_constants(PyObject *module, GType enum_type,
		       const gchar *strip_prefix)
{
    GEnumClass *eclass;
    guint i;

    if (!G_TYPE_IS_ENUM(enum_type)) {
	if (G_TYPE_IS_FLAGS(enum_type))	/* See bug #136204 */
	    pyg_flags_add_constants(module, enum_type, strip_prefix);
	else
	    g_warning("`%s' is not an enum type", g_type_name(enum_type));
	return;
    }
    g_return_if_fail (strip_prefix != NULL);

    eclass = G_ENUM_CLASS(g_type_class_ref(enum_type));

    for (i = 0; i < eclass->n_values; i++) {
	const gchar *name = eclass->values[i].value_name;
	gint value = eclass->values[i].value;

	PyModule_AddIntConstant(module,
				(char*) pyg_constant_strip_prefix(name, strip_prefix),
				(long) value);
    }

    g_type_class_unref(eclass);
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
pyg_flags_add_constants(PyObject *module, GType flags_type,
			const gchar *strip_prefix)
{
    GFlagsClass *fclass;
    guint i;

    if (!G_TYPE_IS_FLAGS(flags_type)) {
	if (G_TYPE_IS_ENUM(flags_type))	/* See bug #136204 */
	    pyg_enum_add_constants(module, flags_type, strip_prefix);
	else
	    g_warning("`%s' is not an flags type", g_type_name(flags_type));
	return;
    }
    g_return_if_fail (strip_prefix != NULL);

    fclass = G_FLAGS_CLASS(g_type_class_ref(flags_type));

    for (i = 0; i < fclass->n_values; i++) {
	const gchar *name = fclass->values[i].value_name;
	guint value = fclass->values[i].value;

	PyModule_AddIntConstant(module,
				(char*) pyg_constant_strip_prefix(name, strip_prefix),
				(long) value);
    }

    g_type_class_unref(fclass);
}

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
pyg_parse_constructor_args(GType        obj_type,
                           char       **arg_names,
                           char       **prop_names,
                           GParameter  *params,
                           guint       *nparams,
                           PyObject   **py_args)
{
    guint arg_i, param_i;
    GObjectClass *oclass;

    oclass = g_type_class_ref(obj_type);
    g_return_val_if_fail(oclass, FALSE);

    for (param_i = arg_i = 0; arg_names[arg_i]; ++arg_i) {
        GParamSpec *spec;
        if (!py_args[arg_i])
            continue;
        spec = g_object_class_find_property(oclass, prop_names[arg_i]);
        params[param_i].name = prop_names[arg_i];
        g_value_init(&params[param_i].value, spec->value_type);
        if (pyg_value_from_pyobject(&params[param_i].value, py_args[arg_i]) == -1) {
            int i;
            PyErr_Format(PyExc_TypeError, "could not convert parameter '%s' of type '%s'",
                         arg_names[arg_i], g_type_name(spec->value_type));
            g_type_class_unref(oclass);
            for (i = 0; i < param_i; ++i)
                g_value_unset(&params[i].value);
            return FALSE;
        }
        ++param_i;
    }
    g_type_class_unref(oclass);
    *nparams = param_i;
    return TRUE;
}

PyObject *
pyg_integer_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *result;
    gboolean t;

    switch (op) {
    case Py_EQ: t = PYGLIB_PyLong_AS_LONG(v) == PYGLIB_PyLong_AS_LONG(w); break;
    case Py_NE: t = PYGLIB_PyLong_AS_LONG(v) != PYGLIB_PyLong_AS_LONG(w); break;
    case Py_LE: t = PYGLIB_PyLong_AS_LONG(v) <= PYGLIB_PyLong_AS_LONG(w); break;
    case Py_GE: t = PYGLIB_PyLong_AS_LONG(v) >= PYGLIB_PyLong_AS_LONG(w); break;
    case Py_LT: t = PYGLIB_PyLong_AS_LONG(v) <  PYGLIB_PyLong_AS_LONG(w); break;
    case Py_GT: t = PYGLIB_PyLong_AS_LONG(v) >  PYGLIB_PyLong_AS_LONG(w); break;
    default: g_assert_not_reached();
    }

    result = t ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

static void
_log_func(const gchar *log_domain,
          GLogLevelFlags log_level,
          const gchar *message,
          gpointer user_data)
{
    if (G_LIKELY(Py_IsInitialized()))
    {
	PyGILState_STATE state;
	PyObject* warning = user_data;

	state = pyglib_gil_state_ensure();
	PyErr_Warn(warning, (char *) message);
	pyglib_gil_state_release(state);
    } else
        g_log_default_handler(log_domain, log_level, message, user_data);
}

static void
add_warning_redirection(const char *domain,
                        PyObject   *warning)
{
    g_return_if_fail(domain != NULL);
    g_return_if_fail(warning != NULL);

    if (!log_handlers_disabled)
    {
	guint handler;
	gpointer old_handler;

	if (!log_handlers)
	    log_handlers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	if ((old_handler = g_hash_table_lookup(log_handlers, domain)))
	    g_log_remove_handler(domain, GPOINTER_TO_UINT(old_handler));

	handler = g_log_set_handler(domain, G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING,
	                            _log_func, warning);
	g_hash_table_insert(log_handlers, g_strdup(domain), GUINT_TO_POINTER(handler));
    }
}

static void
remove_handler(gpointer domain,
               gpointer handler,
	       gpointer unused)
{
    g_log_remove_handler(domain, GPOINTER_TO_UINT(handler));
}

static void
disable_warning_redirections(void)
{
    log_handlers_disabled = TRUE;

    if (log_handlers)
    {
	g_hash_table_foreach(log_handlers, remove_handler, NULL);
	g_hash_table_destroy(log_handlers);
	log_handlers = NULL;
    }
}

/* ----------------- gobject module initialisation -------------- */

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
  pyg_register_boxed,
  pyg_boxed_new,

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

  &PyGParamSpec_Type,
  pyg_param_spec_new,
  pyg_param_spec_from_object,

  pyg_pyobj_to_unichar_conv,
  pyg_parse_constructor_args,
  pyg_param_gvalue_as_pyobject,
  pyg_param_gvalue_from_pyobject,

  &PyGEnum_Type,
  pyg_enum_add,
  pyg_enum_from_gtype,

  &PyGFlags_Type,
  pyg_flags_add,
  pyg_flags_from_gtype,

  /* threads_enabled */
#ifdef DISABLE_THREADING
  FALSE,
#else
  TRUE,
#endif

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

/* for addon libraries ... */
static void
pygobject_register_api(PyObject *d)
{
    PyObject *api;

    api = PYGLIB_CPointer_WrapPointer(&pygobject_api_functions, "gobject._PyGObject_API");
    PyDict_SetItemString(d, "_PyGObject_API", api);
    Py_DECREF(api);
}

/* some constants */
static void
pygobject_register_constants(PyObject *m)
{
    /* PyFloat_ return a new ref, and add object takes the ref */
    PyModule_AddObject(m,       "G_MINFLOAT", PyFloat_FromDouble(G_MINFLOAT));
    PyModule_AddObject(m,       "G_MAXFLOAT", PyFloat_FromDouble(G_MAXFLOAT));
    PyModule_AddObject(m,       "G_MINDOUBLE", PyFloat_FromDouble(G_MINDOUBLE));
    PyModule_AddObject(m,       "G_MAXDOUBLE", PyFloat_FromDouble(G_MAXDOUBLE));
    PyModule_AddIntConstant(m,  "G_MINSHORT", G_MINSHORT);
    PyModule_AddIntConstant(m,  "G_MAXSHORT", G_MAXSHORT);
    PyModule_AddIntConstant(m,  "G_MAXUSHORT", G_MAXUSHORT);
    PyModule_AddIntConstant(m,  "G_MININT", G_MININT);
    PyModule_AddIntConstant(m,  "G_MAXINT", G_MAXINT);
    PyModule_AddObject(m,       "G_MAXUINT", PyLong_FromUnsignedLong(G_MAXUINT));
    PyModule_AddObject(m,       "G_MINLONG", PyLong_FromLong(G_MINLONG));
    PyModule_AddObject(m,       "G_MAXLONG", PyLong_FromLong(G_MAXLONG));
    PyModule_AddObject(m,       "G_MAXULONG", PyLong_FromUnsignedLong(G_MAXULONG));
    PyModule_AddObject(m,       "G_MAXSIZE", PyLong_FromSize_t(G_MAXSIZE));
    PyModule_AddObject(m,       "G_MAXSSIZE", PyLong_FromSsize_t(G_MAXSSIZE));
    PyModule_AddObject(m,       "G_MINSSIZE", PyLong_FromSsize_t(G_MINSSIZE));
    PyModule_AddObject(m,       "G_MINOFFSET", PyLong_FromLongLong(G_MINOFFSET));
    PyModule_AddObject(m,       "G_MAXOFFSET", PyLong_FromLongLong(G_MAXOFFSET));

    PyModule_AddIntConstant(m, "SIGNAL_RUN_FIRST", G_SIGNAL_RUN_FIRST);
    PyModule_AddIntConstant(m, "PARAM_READWRITE", G_PARAM_READWRITE);

    /* The rest of the types are set in __init__.py */
    PyModule_AddObject(m, "TYPE_INVALID", pyg_type_wrapper_new(G_TYPE_INVALID));
    PyModule_AddObject(m, "TYPE_GSTRING", pyg_type_wrapper_new(G_TYPE_GSTRING));
}

/* features */
static void
pygobject_register_features(PyObject *d)
{
    PyObject *features;

    features = PyDict_New();
    PyDict_SetItemString(features, "generic-c-marshaller", Py_True);
    PyDict_SetItemString(d, "features", features);
    Py_DECREF(features);
}

static void
pygobject_register_version_tuples(PyObject *d)
{
    PyObject *tuple;

    /* pygobject version */
    tuple = Py_BuildValue ("(iii)",
			   PYGOBJECT_MAJOR_VERSION,
			   PYGOBJECT_MINOR_VERSION,
			   PYGOBJECT_MICRO_VERSION);
    PyDict_SetItemString(d, "pygobject_version", tuple);
}

static void
pygobject_register_warnings(PyObject *d)
{
    PyObject *warning;

    warning = PyErr_NewException("gobject.Warning", PyExc_Warning, NULL);
    PyDict_SetItemString(d, "Warning", warning);
    add_warning_redirection("GLib", warning);
    add_warning_redirection("GLib-GObject", warning);
    add_warning_redirection("GThread", warning);
}


PYGLIB_MODULE_START(_gobject, "_gobject")
{
    PyObject *d;

    d = PyModule_GetDict(module);
    pygobject_register_api(d);
    pygobject_register_constants(module);
    pygobject_register_features(d);
    pygobject_register_version_tuples(d);
    pygobject_register_warnings(d);
    pygobject_type_register_types(d);
    pygobject_object_register_types(d);
    pygobject_interface_register_types(d);
    pygobject_paramspec_register_types(d);
    pygobject_boxed_register_types(d);
    pygobject_pointer_register_types(d);
    pygobject_enum_register_types(d);
    pygobject_flags_register_types(d);
}
PYGLIB_MODULE_END
